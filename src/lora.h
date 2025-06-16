/**
 * @file lora.h
 * @brief RYLR998 LoRa Module Driver Interface
 * @version 1.0
 * @date 2025-06-15
 * @author Kevin Thomas
 *
 * This header file provides a comprehensive interface for controlling the RYLR998
 * LoRa module using UART communication on the Raspberry Pi Pico. The driver
 * supports AT command communication, message transmission/reception, and
 * configuration of LoRa parameters.
 *
 * The RYLR998 is a LoRa transceiver module that communicates via UART using
 * AT commands. It operates in the 433/470/868/915 MHz frequency bands and
 * supports various spreading factors, bandwidths, and coding rates.
 *
 * Compile-time Configuration:
 * - Define LORA_TRANSMITTER_MODE for button-controlled transmitter
 * - Define LORA_RECEIVER_MODE for stepper motor controller (default)
 *
 * Hardware Requirements:
 * - RYLR998 LoRa module
 * - UART1 connection (GP4=TX, GP5=RX by default)
 * - 3.3V power supply
 * - Optional: external antenna
 *
 * Key Features:
 * - Long range communication (up to 3km line of sight)
 * - Low power consumption
 * - Configurable parameters (frequency, power, spreading factor)
 * - Point-to-point and broadcast communication
 * - Network ID and device address support
 *
 * @copyright Copyright (c) 2025 Kevin Thomas
 */

#ifndef LORA_H
#define LORA_H

// Compile-time mode selection (define one)
// #define LORA_TRANSMITTER_MODE    // Button-controlled remote transmitter
// #define LORA_RECEIVER_MODE       // Stepper motor controller (default)

// Default to receiver mode if nothing specified
#if !defined(LORA_TRANSMITTER_MODE) && !defined(LORA_RECEIVER_MODE)
#define LORA_RECEIVER_MODE
#endif

#include "pico/stdlib.h"
#include "hardware/uart.h"

/**
 * @brief Maximum length for LoRa message payload
 */
#define LORA_MAX_MESSAGE_LENGTH 240

/**
 * @brief Maximum length for AT command responses
 */
#define LORA_MAX_RESPONSE_LENGTH 256

/**
 * @brief Default UART baud rate for RYLR998
 */
#define LORA_DEFAULT_BAUD_RATE 9600

/**
 * @brief Command timeout in milliseconds
 */
#define LORA_COMMAND_TIMEOUT_MS 2000

/**
 * @brief Response timeout in milliseconds
 */
#define LORA_RESPONSE_TIMEOUT_MS 1000

/**
 * @brief LoRa module status enumeration
 */
typedef enum
{
    LORA_STATUS_OK = 0,          ///< Operation successful
    LORA_STATUS_ERROR,           ///< General error
    LORA_STATUS_TIMEOUT,         ///< Command timeout
    LORA_STATUS_INVALID_PARAM,   ///< Invalid parameter
    LORA_STATUS_NOT_INITIALIZED, ///< Module not initialized
    LORA_STATUS_UART_ERROR       ///< UART communication error
} lora_status_t;

/**
 * @brief LoRa power level enumeration (0-15)
 */
typedef enum
{
    LORA_POWER_0 = 0,   ///< Minimum power
    LORA_POWER_5 = 5,   ///< Low power
    LORA_POWER_10 = 10, ///< Medium power
    LORA_POWER_15 = 15  ///< Maximum power
} lora_power_t;

/**
 * @brief LoRa spreading factor enumeration
 */
typedef enum
{
    LORA_SF_7 = 7,   ///< SF7 - fastest, shortest range
    LORA_SF_8 = 8,   ///< SF8
    LORA_SF_9 = 9,   ///< SF9 (default)
    LORA_SF_10 = 10, ///< SF10
    LORA_SF_11 = 11  ///< SF11 - slowest, longest range (max valid)
} lora_spreading_factor_t;

/**
 * @brief LoRa bandwidth enumeration
 */
typedef enum
{
    LORA_BW_7_8 = 0,   ///< 7.8 kHz
    LORA_BW_10_4 = 1,  ///< 10.4 kHz
    LORA_BW_15_6 = 2,  ///< 15.6 kHz
    LORA_BW_20_8 = 3,  ///< 20.8 kHz
    LORA_BW_31_25 = 4, ///< 31.25 kHz
    LORA_BW_41_7 = 5,  ///< 41.7 kHz
    LORA_BW_62_5 = 6,  ///< 62.5 kHz
    LORA_BW_125 = 7,   ///< 125 kHz
    LORA_BW_250 = 8,   ///< 250 kHz
    LORA_BW_500 = 9    ///< 500 kHz
} lora_bandwidth_t;

/**
 * @brief LoRa coding rate enumeration
 */
typedef enum
{
    LORA_CR_4_5 = 1, ///< 4/5
    LORA_CR_4_6 = 2, ///< 4/6
    LORA_CR_4_7 = 3, ///< 4/7
    LORA_CR_4_8 = 4  ///< 4/8
} lora_coding_rate_t;

/**
 * @brief LoRa module configuration structure
 */
typedef struct
{
    uart_inst_t *uart;              ///< UART instance
    uint tx_pin;                    ///< TX GPIO pin
    uint rx_pin;                    ///< RX GPIO pin
    uint baud_rate;                 ///< UART baud rate
    uint16_t network_id;            ///< Network ID (0-65535)
    uint16_t device_address;        ///< Device address (0-65535)
    uint32_t frequency;             ///< Frequency in Hz
    lora_power_t power;             ///< Transmission power
    lora_spreading_factor_t sf;     ///< Spreading factor
    lora_bandwidth_t bandwidth;     ///< Bandwidth
    lora_coding_rate_t coding_rate; ///< Coding rate
    bool initialized;               ///< Initialization status
} lora_config_t;

/**
 * @brief LoRa received message structure
 */
typedef struct
{
    uint16_t sender_address;               ///< Sender address
    uint8_t rssi;                          ///< Signal strength (RSSI)
    uint8_t payload_length;                ///< Message payload length
    char payload[LORA_MAX_MESSAGE_LENGTH]; ///< Message payload
} lora_message_t;

/**
 * @brief Message handler callback function type
 *
 * @param message Pointer to received message structure
 * @param user_data User-defined data pointer
 */
typedef void (*lora_message_handler_t)(const lora_message_t *message, void *user_data);

// Function declarations

/**
 * @brief Initialize the LoRa module with default configuration
 *
 * @param config Pointer to LoRa configuration structure
 * @param uart_inst UART instance to use (uart0 or uart1)
 * @param tx_pin GPIO pin for UART TX
 * @param rx_pin GPIO pin for UART RX
 * @return lora_status_t Status of initialization
 */
lora_status_t lora_init(lora_config_t *config, uart_inst_t *uart_inst, uint tx_pin, uint rx_pin);

/**
 * @brief Initialize LoRa module with custom parameters
 *
 * @param config Pointer to LoRa configuration structure
 * @param uart_inst UART instance to use
 * @param tx_pin GPIO pin for UART TX
 * @param rx_pin GPIO pin for UART RX
 * @param network_id Network ID
 * @param device_address Device address
 * @param frequency Frequency in Hz
 * @param power Transmission power
 * @return lora_status_t Status of initialization
 */
lora_status_t lora_init_custom(lora_config_t *config, uart_inst_t *uart_inst,
                               uint tx_pin, uint rx_pin, uint16_t network_id,
                               uint16_t device_address, uint32_t frequency,
                               lora_power_t power);

/**
 * @brief Test if LoRa module is responding
 *
 * @param config Pointer to LoRa configuration structure
 * @return lora_status_t Status of test
 */
lora_status_t lora_test(lora_config_t *config);

/**
 * @brief Send a message to a specific address
 *
 * @param config Pointer to LoRa configuration structure
 * @param address Target device address
 * @param message Message to send
 * @param length Message length
 * @return lora_status_t Status of transmission
 */
lora_status_t lora_send_message(lora_config_t *config, uint16_t address,
                                const char *message, uint8_t length);

/**
 * @brief Send a broadcast message to all devices
 *
 * @param config Pointer to LoRa configuration structure
 * @param message Message to send
 * @param length Message length
 * @return lora_status_t Status of transmission
 */
lora_status_t lora_broadcast_message(lora_config_t *config, const char *message, uint8_t length);

/**
 * @brief Check for received messages (non-blocking)
 *
 * @param config Pointer to LoRa configuration structure
 * @param message Pointer to message structure to fill
 * @return lora_status_t Status of reception
 */
lora_status_t lora_receive_message(lora_config_t *config, lora_message_t *message);

/**
 * @brief Set message handler callback for automatic message processing
 *
 * @param config Pointer to LoRa configuration structure
 * @param handler Message handler callback function
 * @param user_data User data to pass to callback
 * @return lora_status_t Status of operation
 */
lora_status_t lora_set_message_handler(lora_config_t *config, lora_message_handler_t handler, void *user_data);

/**
 * @brief Process incoming messages (call this regularly in main loop)
 *
 * @param config Pointer to LoRa configuration structure
 * @return lora_status_t Status of processing
 */
lora_status_t lora_process_messages(lora_config_t *config);

/**
 * @brief Configure LoRa parameters
 *
 * @param config Pointer to LoRa configuration structure
 * @param frequency Frequency in Hz
 * @param power Transmission power
 * @param sf Spreading factor
 * @param bandwidth Bandwidth
 * @param coding_rate Coding rate
 * @return lora_status_t Status of configuration
 */
lora_status_t lora_configure(lora_config_t *config, uint32_t frequency,
                             lora_power_t power, lora_spreading_factor_t sf,
                             lora_bandwidth_t bandwidth, lora_coding_rate_t coding_rate);

/**
 * @brief Reset the LoRa module
 *
 * @param config Pointer to LoRa configuration structure
 * @return lora_status_t Status of reset
 */
lora_status_t lora_reset(lora_config_t *config);

/**
 * @brief Get LoRa module version information
 *
 * @param config Pointer to LoRa configuration structure
 * @param version Buffer to store version string
 * @param max_length Maximum length of version buffer
 * @return lora_status_t Status of operation
 */
lora_status_t lora_get_version(lora_config_t *config, char *version, uint8_t max_length);

/**
 * @brief Send a raw AT command to the LoRa module
 *
 * @param config Pointer to LoRa configuration structure
 * @param command AT command to send
 * @param response Buffer to store response
 * @param max_response_len Maximum length of response buffer
 * @return lora_status_t Status of operation
 */
lora_status_t lora_send_at_command(lora_config_t *config, const char *command, char *response, uint8_t max_response_len);

/**
 * @brief Put LoRa module into sleep mode
 *
 * @param config Pointer to LoRa configuration structure
 * @return lora_status_t Status of operation
 */
lora_status_t lora_sleep(lora_config_t *config);

/**
 * @brief Wake up LoRa module from sleep mode
 *
 * @param config Pointer to LoRa configuration structure
 * @return lora_status_t Status of operation
 */
lora_status_t lora_wake(lora_config_t *config);

/**
 * @brief Check if a message contains "ON" command
 *
 * @param message Message payload to check
 * @return bool True if message contains ON command
 */
bool lora_is_on_command(const char *message);

/**
 * @brief Check if a message contains "OFF" command
 *
 * @param message Message payload to check
 * @return bool True if message contains OFF command
 */
bool lora_is_off_command(const char *message);

// Button control functions for transmitter mode

/**
 * @brief Button configuration structure
 */
typedef struct
{
    uint pin;           ///< GPIO pin number
    bool last_state;    ///< Last button state
    uint32_t last_time; ///< Last debounce time
} button_t;

/**
 * @brief Initialize button with internal pull-up resistor
 *
 * @param button Pointer to button structure
 * @param pin GPIO pin number
 */
void lora_button_init(button_t *button, uint pin);

/**
 * @brief Check if button is pressed (with debouncing)
 *
 * @param button Pointer to button structure
 * @return bool True if button was pressed
 */
bool lora_button_pressed(button_t *button);

/**
 * @brief Initialize all buttons for transmitter mode
 *
 * @param buttons Array of button structures (2 buttons expected)
 */
void lora_buttons_init_all(button_t buttons[2]);

#endif // LORA_H
