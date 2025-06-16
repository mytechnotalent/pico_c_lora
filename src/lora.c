/**
 * @file lora.c
 * @brief RYLR998 LoRa Module Driver Implementation
 * @version 1.0
 * @date 2025-06-15
 * @author Kevin Thomas
 *
 * This source file implements the RYLR998 LoRa module driver for the Raspberry Pi Pico.
 * It provides AT command communication, message handling, and configuration management
 * for the LoRa transceiver module using UART1.
 *
 * The implementation supports:
 * - AT command communication with timeout handling
 * - Message transmission and reception
 * - Configuration of LoRa parameters
 * - Asynchronous message processing with callbacks
 * - Command parsing for stepper motor control
 *
 * @copyright Copyright (c) 2025 Kevin Thomas
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lora.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

// Internal constants
#define AT_COMMAND_BUFFER_SIZE 128
#define RESPONSE_BUFFER_SIZE 256
#define MAX_RETRY_COUNT 3
#define UART_RX_BUFFER_SIZE 512

// Circular buffer for UART interrupt handling
typedef struct
{
    char buffer[UART_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile bool overflow;
} uart_rx_buffer_t;

// Internal state structure
typedef struct
{
    lora_message_handler_t message_handler;
    void *user_data;
    char rx_buffer[RESPONSE_BUFFER_SIZE];
    uint8_t rx_index;
    bool command_pending;
    uart_rx_buffer_t uart_buffer;
    lora_config_t *config; // Store config for interrupt handler
} lora_internal_state_t;

static lora_internal_state_t internal_state = {0};

// Forward declarations
static lora_status_t send_at_command(lora_config_t *config, const char *command, char *response, uint8_t max_response_len);
static bool wait_for_response(lora_config_t *config, char *response, uint8_t max_len, uint32_t timeout_ms);
static void parse_received_message(const char *response, lora_message_t *message);
static bool is_response_ok(const char *response);
static void uart_rx_interrupt_handler();
static void uart_buffer_init(uart_rx_buffer_t *buffer);
static bool uart_buffer_put(uart_rx_buffer_t *buffer, char c);
static bool uart_buffer_get(uart_rx_buffer_t *buffer, char *c);
static uint16_t uart_buffer_available(uart_rx_buffer_t *buffer);
static bool uart_buffer_get_line(uart_rx_buffer_t *buffer, char *line, uint16_t max_len);
static void clear_uart_buffer(lora_config_t *config);

lora_status_t lora_init(lora_config_t *config, uart_inst_t *uart_inst, uint tx_pin, uint rx_pin)
{
    return lora_init_custom(config, uart_inst, tx_pin, rx_pin, 0, 0, 433000000, LORA_POWER_10);
}

lora_status_t lora_init_custom(lora_config_t *config, uart_inst_t *uart_inst,
                               uint tx_pin, uint rx_pin, uint16_t network_id,
                               uint16_t device_address, uint32_t frequency,
                               lora_power_t power)
{
    if (!config || !uart_inst)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    // Initialize configuration structure
    memset(config, 0, sizeof(lora_config_t));
    config->uart = uart_inst;
    config->tx_pin = tx_pin;
    config->rx_pin = rx_pin;
    config->baud_rate = LORA_DEFAULT_BAUD_RATE;
    config->network_id = network_id;
    config->device_address = device_address;
    config->frequency = frequency;
    config->power = power;
    config->sf = LORA_SF_9; // Default spreading factor (valid range: 5-11)
    config->bandwidth = LORA_BW_125;
    config->coding_rate = LORA_CR_4_5;
    config->initialized = false;

    // Initialize UART
    uart_init(config->uart, config->baud_rate);

    // Configure GPIO pins
    gpio_set_function(config->tx_pin, GPIO_FUNC_UART);
    gpio_set_function(config->rx_pin, GPIO_FUNC_UART);

    // Configure UART parameters
    uart_set_hw_flow(config->uart, false, false);
    uart_set_format(config->uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(config->uart, true);

    // Clear internal state and initialize interrupt buffer
    memset(&internal_state, 0, sizeof(lora_internal_state_t));
    internal_state.config = config; // Store config for interrupt handler
    uart_buffer_init(&internal_state.uart_buffer);

    // Set up UART interrupt for RX
    int uart_irq = (config->uart == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, uart_rx_interrupt_handler);
    irq_set_enabled(uart_irq, true);

    // Enable UART RX interrupt
    uart_set_irq_enables(config->uart, true, false); // RX interrupt enabled, TX disabled

    printf("LoRa: UART interrupt enabled for reliable message reception\n");

    // Wait for module to stabilize
    sleep_ms(1000);

    // Test communication
    printf("LoRa: Testing communication with AT command...\n");
    lora_status_t status = lora_test(config);
    if (status != LORA_STATUS_OK)
    {
        printf("LoRa: ❌ CRITICAL ERROR - Module not responding to AT commands!\n");
        printf("LoRa: Check: 1) 3.3V power, 2) Wiring, 3) Baud rate (9600)\n");
        printf("LoRa: Expected response: +OK, but got no response or error\n");
        return status;
    }
    printf("LoRa: ✅ AT communication working\n");

    // Configure network parameters
    if (network_id != 0)
    {
        char command[32];
        char response[64];

        // Validate Network ID range (3-15, 18 according to datasheet)
        if (network_id != 18 && (network_id < 3 || network_id > 15))
        {
            printf("LoRa: ❌ INVALID Network ID: %d\n", network_id);
            printf("LoRa: 📖 Valid Network IDs: 3-15, 18 (default: 18)\n");
            return LORA_STATUS_ERROR;
        }

        printf("LoRa: Setting Network ID to %d...\n", network_id);
        snprintf(command, sizeof(command), "AT+NETWORKID=%d", network_id);
        status = send_at_command(config, command, response, sizeof(response));
        if (status != LORA_STATUS_OK || !is_response_ok(response))
        {
            printf("LoRa: ❌ Failed to set network ID - Response: %s\n", response);
            return LORA_STATUS_ERROR;
        }
        printf("LoRa: ✅ Network ID set successfully to %d\n", network_id);
    }

    if (device_address != 0)
    {
        char command[32];
        char response[64];

        printf("LoRa: Setting Device Address to %d...\n", device_address);
        snprintf(command, sizeof(command), "AT+ADDRESS=%d", device_address);
        status = send_at_command(config, command, response, sizeof(response));
        if (status != LORA_STATUS_OK || !is_response_ok(response))
        {
            printf("LoRa: ❌ Failed to set device address - Response: %s\n", response);
            return LORA_STATUS_ERROR;
        }
        printf("LoRa: ✅ Device address set successfully\n");
    }

    // Configure frequency and power
    printf("LoRa: Setting frequency to %ld Hz and power to %d...\n", frequency, power);
    status = lora_configure(config, frequency, power, config->sf, config->bandwidth, config->coding_rate);
    if (status != LORA_STATUS_OK)
    {
        printf("LoRa: ❌ Failed to configure frequency/power - check settings!\n");
        printf("LoRa: Ensure frequency is 915000000 Hz for US operation\n");
        return status;
    }
    printf("LoRa: ✅ Frequency and power configured successfully\n");

    config->initialized = true;
    printf("LoRa: Module initialized successfully\n");
    printf("LoRa: Network ID: %d, Address: %d, Freq: %ld Hz\n",
           network_id, device_address, frequency);

    return LORA_STATUS_OK;
}

lora_status_t lora_test(lora_config_t *config)
{
    if (!config)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    char response[64];
    lora_status_t status = send_at_command(config, "AT", response, sizeof(response));

    if (status == LORA_STATUS_OK && is_response_ok(response))
    {
        printf("LoRa: Module responding to AT commands\n");
        return LORA_STATUS_OK;
    }

    printf("LoRa: Module not responding (status: %d, response: %s)\n", status, response);
    return LORA_STATUS_ERROR;
}

lora_status_t lora_send_message(lora_config_t *config, uint16_t address,
                                const char *message, uint8_t length)
{
    if (!config || !config->initialized || !message || length == 0)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    if (length > LORA_MAX_MESSAGE_LENGTH)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    char command[LORA_MAX_MESSAGE_LENGTH + 32];
    char response[64];

    // Format: AT+SEND=<address>,<length>,<message>
    snprintf(command, sizeof(command), "AT+SEND=%d,%d,%s", address, length, message);

    lora_status_t status = send_at_command(config, command, response, sizeof(response));

    if (status == LORA_STATUS_OK && is_response_ok(response))
    {
        printf("LoRa: Message sent to address %d: %s\n", address, message);
        return LORA_STATUS_OK;
    }

    printf("LoRa: Failed to send message (status: %d)\n", status);
    return status;
}

lora_status_t lora_broadcast_message(lora_config_t *config, const char *message, uint8_t length)
{
    // Use broadcast address 65535
    return lora_send_message(config, 65535, message, length);
}

lora_status_t lora_receive_message(lora_config_t *config, lora_message_t *message)
{
    if (!config || !config->initialized || !message)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    char response[RESPONSE_BUFFER_SIZE];

    // Check if there's a complete line in the interrupt buffer
    if (!uart_buffer_get_line(&internal_state.uart_buffer, response, sizeof(response)))
    {
        return LORA_STATUS_ERROR; // No complete message available
    }

    printf("LoRa: 🔍 RAW UART DATA: '%s' (len=%d)\n", response, strlen(response));

    // Print hex dump of the data for detailed analysis
    printf("LoRa: HEX: ");
    for (int i = 0; response[i] != '\0'; i++)
    {
        printf("%02X ", (unsigned char)response[i]);
    }
    printf("\n");

    // Check if this is a received message (format: +RCV=<address>,<length>,<data>,<rssi>)
    if (strncmp(response, "+RCV=", 5) == 0)
    {
        printf("LoRa: 📨 INCOMING LORA MESSAGE DETECTED!\n");
        parse_received_message(response, message);
        printf("LoRa: ✅ LoRa message received from %d: '%s' (RSSI: %d)\n",
               message->sender_address, message->payload, message->rssi);
        return LORA_STATUS_OK;
    }
    else if (strncmp(response, "+OK", 3) == 0)
    {
        printf("LoRa: 📤 AT command response: '%s' (ignoring)\n", response);
    }
    else if (strncmp(response, "+ERR", 4) == 0)
    {
        printf("LoRa: ❌ AT command error: '%s'\n", response);
    }
    else
    {
        printf("LoRa: ❓ Unknown response: '%s' (not +RCV, +OK, or +ERR)\n", response);
    }

    return LORA_STATUS_ERROR;
}

lora_status_t lora_set_message_handler(lora_config_t *config, lora_message_handler_t handler, void *user_data)
{
    if (!config)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    internal_state.message_handler = handler;
    internal_state.user_data = user_data;

    return LORA_STATUS_OK;
}

lora_status_t lora_process_messages(lora_config_t *config)
{
    if (!config || !config->initialized)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    lora_message_t message;
    lora_status_t status = lora_receive_message(config, &message);

    if (status == LORA_STATUS_OK)
    {
        if (internal_state.message_handler)
        {
            printf("LoRa: 🔧 Calling message handler...\n");
            internal_state.message_handler(&message, internal_state.user_data);
        }
        else
        {
            printf("LoRa: ❌ No message handler set!\n");
        }
    }

    return status;
}

lora_status_t lora_configure(lora_config_t *config, uint32_t frequency,
                             lora_power_t power, lora_spreading_factor_t sf,
                             lora_bandwidth_t bandwidth, lora_coding_rate_t coding_rate)
{
    if (!config)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    char command[64];
    char response[64];
    lora_status_t status;

    // Set frequency (in Hz, not MHz)
    snprintf(command, sizeof(command), "AT+BAND=%ld", frequency);
    status = send_at_command(config, command, response, sizeof(response));
    if (status != LORA_STATUS_OK || !is_response_ok(response))
    {
        printf("LoRa: ❌ Failed to set frequency - Response: %s\n", response);
        return LORA_STATUS_ERROR;
    }

    // Set transmission power (correct command is AT+CRFOP)
    snprintf(command, sizeof(command), "AT+CRFOP=%d", (int)power);
    status = send_at_command(config, command, response, sizeof(response));
    if (status != LORA_STATUS_OK || !is_response_ok(response))
    {
        printf("LoRa: ❌ Failed to set power - Response: %s\n", response);
        return LORA_STATUS_ERROR;
    }

    // Set spreading factor
    snprintf(command, sizeof(command), "AT+PARAMETER=%d,%d,%d,%d",
             (int)sf, (int)bandwidth, (int)coding_rate, 8); // 8 = preamble length
    status = send_at_command(config, command, response, sizeof(response));
    if (status != LORA_STATUS_OK || !is_response_ok(response))
    {
        printf("LoRa: ❌ Failed to set parameters - Response: %s\n", response);
        return LORA_STATUS_ERROR;
    }

    // Update configuration
    config->frequency = frequency;
    config->power = power;
    config->sf = sf;
    config->bandwidth = bandwidth;
    config->coding_rate = coding_rate;

    printf("LoRa: Configuration updated - Freq: %ld Hz, Power: %d, SF: %d\n",
           frequency, power, sf);

    return LORA_STATUS_OK;
}

lora_status_t lora_reset(lora_config_t *config)
{
    if (!config)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    char response[64];
    lora_status_t status = send_at_command(config, "AT+RESET", response, sizeof(response));

    if (status == LORA_STATUS_OK && is_response_ok(response))
    {
        sleep_ms(2000); // Wait for reset to complete
        config->initialized = false;
        printf("LoRa: Module reset successfully\n");
        return LORA_STATUS_OK;
    }
    else
    {
        printf("LoRa: ❌ Failed to reset module - Response: %s\n", response);
        return LORA_STATUS_ERROR;
    }
}

lora_status_t lora_get_version(lora_config_t *config, char *version, uint8_t max_length)
{
    if (!config || !version || max_length == 0)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    char response[128];
    lora_status_t status = send_at_command(config, "AT+VER?", response, sizeof(response));

    if (status == LORA_STATUS_OK)
    {
        strncpy(version, response, max_length - 1);
        version[max_length - 1] = '\0';
    }

    return status;
}

lora_status_t lora_sleep(lora_config_t *config)
{
    if (!config)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    char response[64];
    lora_status_t status = send_at_command(config, "AT+MODE=1", response, sizeof(response));
    if (status != LORA_STATUS_OK || !is_response_ok(response))
    {
        printf("LoRa: ❌ Failed to set sleep mode - Response: %s\n", response);
        return LORA_STATUS_ERROR;
    }
    return LORA_STATUS_OK;
}

lora_status_t lora_wake(lora_config_t *config)
{
    if (!config)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    char response[64];
    lora_status_t status = send_at_command(config, "AT+MODE=0", response, sizeof(response));
    if (status != LORA_STATUS_OK || !is_response_ok(response))
    {
        printf("LoRa: ❌ Failed to set wake mode - Response: %s\n", response);
        return LORA_STATUS_ERROR;
    }
    return LORA_STATUS_OK;
}

bool lora_is_on_command(const char *message)
{
    if (!message)
    {
        return false;
    }

    // Check for various "ON" command variations (case insensitive)
    return (strcasecmp(message, "ON") == 0 ||
            strcasecmp(message, "START") == 0 ||
            strcasecmp(message, "MOVE") == 0 ||
            strcasecmp(message, "1") == 0);
}

bool lora_is_off_command(const char *message)
{
    if (!message)
    {
        return false;
    }

    // Check for various "OFF" command variations (case insensitive)
    return (strcasecmp(message, "OFF") == 0 ||
            strcasecmp(message, "STOP") == 0 ||
            strcasecmp(message, "HALT") == 0 ||
            strcasecmp(message, "0") == 0);
}

// Internal helper functions

static lora_status_t send_at_command(lora_config_t *config, const char *command, char *response, uint8_t max_response_len)
{
    if (!config || !command || !response)
    {
        return LORA_STATUS_INVALID_PARAM;
    }

    // Clear any pending data
    clear_uart_buffer(config);

    // Send command
    uart_puts(config->uart, command);
    uart_puts(config->uart, "\r\n");

    printf("LoRa: Sent command: %s\n", command);

    // Wait for response
    bool got_response = wait_for_response(config, response, max_response_len, LORA_COMMAND_TIMEOUT_MS);

    if (!got_response)
    {
        printf("LoRa: ❌ Command TIMEOUT - No response from module!\n");
        printf("LoRa: Check: 1) Module power, 2) Wiring, 3) Baud rate (9600)\n");
        return LORA_STATUS_TIMEOUT;
    }

    printf("LoRa: ✅ Got response: %s\n", response);
    return LORA_STATUS_OK;
}

static bool wait_for_response(lora_config_t *config, char *response, uint8_t max_len, uint32_t timeout_ms)
{
    absolute_time_t timeout = make_timeout_time_ms(timeout_ms);

    while (absolute_time_diff_us(get_absolute_time(), timeout) > 0)
    {
        // Try to get a complete line from the interrupt buffer
        if (uart_buffer_get_line(&internal_state.uart_buffer, response, max_len))
        {
            return true;
        }
        // Remove sleep for maximum speed - busy wait for immediate response
    }

    return false; // Timeout
}

static void parse_received_message(const char *response, lora_message_t *message)
{
    // Format: +RCV=<address>,<length>,<data>,<rssi>
    // Example: +RCV=123,5,HELLO,-45

    if (!response || !message)
    {
        return;
    }

    memset(message, 0, sizeof(lora_message_t));

    const char *ptr = response + 5; // Skip "+RCV="

    // Parse sender address
    message->sender_address = (uint16_t)strtoul(ptr, (char **)&ptr, 10);
    if (*ptr == ',')
        ptr++;

    // Parse length
    message->payload_length = (uint8_t)strtoul(ptr, (char **)&ptr, 10);
    if (*ptr == ',')
        ptr++;

    // Parse payload
    const char *payload_start = ptr;
    const char *payload_end = strchr(ptr, ',');
    if (payload_end)
    {
        uint8_t payload_len = payload_end - payload_start;
        if (payload_len > LORA_MAX_MESSAGE_LENGTH - 1)
        {
            payload_len = LORA_MAX_MESSAGE_LENGTH - 1;
        }
        memcpy(message->payload, payload_start, payload_len);
        message->payload[payload_len] = '\0';
        message->payload_length = payload_len;

        // Parse RSSI
        ptr = payload_end + 1;
        message->rssi = (uint8_t)abs((int)strtol(ptr, NULL, 10));
    }
}

static bool is_response_ok(const char *response)
{
    if (!response)
    {
        return false;
    }

    // Check for error responses first
    if (strstr(response, "+ERR") != NULL || strstr(response, "ERROR") != NULL)
    {
        return false;
    }

    // Check for success responses
    return (strstr(response, "OK") != NULL || strstr(response, "+OK") != NULL);
}

static void clear_uart_buffer(lora_config_t *config)
{
    // Clear hardware UART FIFO
    while (uart_is_readable(config->uart))
    {
        uart_getc(config->uart);
    }

    // Clear interrupt buffer
    uart_buffer_init(&internal_state.uart_buffer);
}

// UART Interrupt Handler and Buffer Management Functions

static void uart_buffer_init(uart_rx_buffer_t *buffer)
{
    buffer->head = 0;
    buffer->tail = 0;
    buffer->overflow = false;
}

static bool uart_buffer_put(uart_rx_buffer_t *buffer, char c)
{
    uint16_t next_head = (buffer->head + 1) % UART_RX_BUFFER_SIZE;

    if (next_head == buffer->tail)
    {
        buffer->overflow = true;
        return false; // Buffer full
    }

    buffer->buffer[buffer->head] = c;
    buffer->head = next_head;
    return true;
}

static bool uart_buffer_get(uart_rx_buffer_t *buffer, char *c)
{
    if (buffer->head == buffer->tail)
    {
        return false; // Buffer empty
    }

    *c = buffer->buffer[buffer->tail];
    buffer->tail = (buffer->tail + 1) % UART_RX_BUFFER_SIZE;
    return true;
}

static uint16_t uart_buffer_available(uart_rx_buffer_t *buffer)
{
    return (buffer->head - buffer->tail + UART_RX_BUFFER_SIZE) % UART_RX_BUFFER_SIZE;
}

static bool uart_buffer_get_line(uart_rx_buffer_t *buffer, char *line, uint16_t max_len)
{
    uint16_t index = 0;
    char c;

    printf("LoRa: 🔍 uart_buffer_get_line: Trying to read line from %d chars...\n", uart_buffer_available(buffer));

    while (index < max_len - 1 && uart_buffer_get(buffer, &c))
    {
        printf("LoRa: Read char: 0x%02X (%c)\n", (unsigned char)c, (c >= 32 && c <= 126) ? c : '.');

        if (c == '\r' || c == '\n')
        {
            if (index > 0)
            {
                line[index] = '\0';
                printf("LoRa: 📝 Complete line found: '%s' (len=%d)\n", line, index);
                return true; // Complete line found
            }
            // Skip empty lines (consecutive \r\n)
            printf("LoRa: Skipping empty line character\n");
            continue;
        }
        line[index++] = c;
    }

    if (index > 0)
    {
        line[index] = '\0';
        printf("LoRa: 📝 Partial line returned: '%s' (len=%d)\n", line, index);
        return true; // Return partial line if buffer runs out
    }

    printf("LoRa: ❌ No complete line available\n");
    return false; // No complete line available
}

// UART interrupt handler
static void uart_rx_interrupt_handler()
{
    uart_inst_t *uart = internal_state.config->uart;
    static uint32_t interrupt_count = 0;

    // Read all available characters
    while (uart_is_readable(uart))
    {
        char c = uart_getc(uart);
        interrupt_count++;

        // Debug: Print first few interrupts to confirm they're working
        if (interrupt_count <= 10 || (interrupt_count % 100) == 0)
        {
            printf("LoRa: UART interrupt #%lu received char: 0x%02X (%c)\n",
                   interrupt_count, (unsigned char)c, (c >= 32 && c <= 126) ? c : '.');
        }

        // Store in circular buffer
        if (!uart_buffer_put(&internal_state.uart_buffer, c))
        {
            printf("LoRa: UART buffer overflow at interrupt #%lu!\n", interrupt_count);
            break;
        }
    }
}

// Button control functions for transmitter mode

void lora_button_init(button_t *button, uint pin)
{
    if (!button)
        return;

    button->pin = pin;
    button->last_state = true; // Released state (pulled high)
    button->last_time = 0;

    // Initialize GPIO with internal pull-up
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin); // Enable internal pull-up resistor

    printf("Button: Initialized pin %d with internal pull-up\n", pin);
}

bool lora_button_pressed(button_t *button)
{
    if (!button)
        return false;

    bool current_state = gpio_get(button->pin);
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Button is pressed when GPIO reads LOW (pulled to ground)
    if (!current_state && button->last_state &&
        (current_time - button->last_time) > 50)
    { // 50ms debounce

        button->last_time = current_time;
        button->last_state = current_state;
        return true;
    }

    button->last_state = current_state;
    return false;
}

void lora_buttons_init_all(button_t buttons[2])
{
    if (!buttons)
        return;

    // Default button pin assignments (can be overridden)
    lora_button_init(&buttons[0], 2); // Button 1 - ON
    lora_button_init(&buttons[1], 3); // Button 2 - OFF

    printf("Button: 2 buttons initialized with internal pull-ups\n");
}

// Public wrapper for AT command sending (for diagnostics)
lora_status_t lora_send_at_command(lora_config_t *config, const char *command, char *response, uint8_t max_response_len)
{
    return send_at_command(config, command, response, max_response_len);
}
