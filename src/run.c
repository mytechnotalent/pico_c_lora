/**
 * @file run.c
 * @brief LED control, stepper motor, and LoRa communication application implementation
 * @version 1.0
 * @date 2025-06-15
 * @author Kevin Thomas
 *
 * This source file implements the main application logic for controlling
 * both LED blinking, stepper motor operations, and LoRa wireless communication
 * on the Raspberry Pi Pico. Provides a unified interface for the combined
 * functionality with remote control capabilities via LoRa commands.
 *
 * @copyright Copyright (c) 2025 Kevin Thomas
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "run.h"

// Constants for LED configuration
// LED only used for LoRa signal reception indication

// Constants for stepper motor configuration
#define NUM_STEPPERS 4
#define STEPPER_DELAY_MS 1       // Minimal 1ms delay for stepper timing, chunked for interrupts
#define STEPPER_DEMO_ANGLE 90.0f // Larger angle for more visible movement
#define STEPPER_PAUSE_MS 0       // NO FUCKING DELAYS
#define STEPPER_CYCLE_INTERVAL 5

// Constants for LoRa configuration
#define LORA_UART_INST uart1
#define LORA_TX_PIN 4
#define LORA_RX_PIN 5
#define LORA_NETWORK_ID 18       // Valid range: 3-15,18 (18 is default)
#define LORA_FREQUENCY 915000000 // 915 MHz (US ISM band)
#define LORA_POWER LORA_POWER_10

#ifdef LORA_TRANSMITTER_MODE
// Transmitter mode configuration
#define LORA_DEVICE_ADDRESS 200        // Transmitter address
#define STEPPER_CONTROLLER_ADDRESS 100 // Target receiver address

// Button pin assignments (using internal pull-ups)
#define BUTTON_ON_PIN 2
#define BUTTON_OFF_PIN 3

// Global variables for transmitter mode
static button_t buttons[2];
#else
// Receiver mode configuration (default)
#define LORA_DEVICE_ADDRESS 100 // Receiver address
#endif

// Global variables for LoRa and stepper control
static lora_config_t lora_config;
static stepper_motor_t *global_steppers = NULL;
static uint global_num_steppers = 0;
static bool stepper_active = false;

// GPIO pin assignments for stepper motors
static const uint stepper_pins[NUM_STEPPERS][4] = {
    {2, 3, 6, 7},     // Stepper Motor 1
    {10, 11, 14, 15}, // Stepper Motor 2
    {18, 19, 20, 21}, // Stepper Motor 3
    {22, 26, 27, 28}  // Stepper Motor 4
};

// LED blink function removed - LED only for signal reception

void control_steppers(stepper_motor_t *steppers, uint num_steppers)
{
    // Create array of pointers for bulk operations
    stepper_motor_t *stepper_ptrs[NUM_STEPPERS];
    for (uint i = 0; i < num_steppers; i++)
    {
        stepper_ptrs[i] = &steppers[i];
    }

    // Continuous clockwise rotation - tiny increments for maximum interrupt responsiveness
    stepper_rotate_multiple_degrees(stepper_ptrs, num_steppers, 1.0f, STEPPER_CW);

    // No debug output - maximum speed for interrupt response
}

/**
 * @brief Initialize all stepper motors with predefined GPIO pins
 *
 * @param steppers Array of stepper motor structures to initialize
 * @param num_steppers Number of steppers to initialize
 * @return true if all motors initialized successfully, false otherwise
 */
static bool init_all_steppers(stepper_motor_t *steppers, uint num_steppers)
{
    if (num_steppers > NUM_STEPPERS)
    {
        printf("Error: Requested %d steppers, but only %d configurations available\n",
               num_steppers, NUM_STEPPERS);
        return false;
    }

    for (uint i = 0; i < num_steppers; i++)
    {
        if (!stepper_init(&steppers[i],
                          stepper_pins[i][0], stepper_pins[i][1],
                          stepper_pins[i][2], stepper_pins[i][3],
                          STEPPER_DELAY_MS))
        {
            printf("Failed to initialize stepper motor %d\n", i + 1);
            return false;
        }
        printf("Stepper motor %d initialized on pins %d,%d,%d,%d\n",
               i + 1, stepper_pins[i][0], stepper_pins[i][1],
               stepper_pins[i][2], stepper_pins[i][3]);
    }

    return true;
}

void lora_message_handler(const lora_message_t *message, void *user_data)
{
    if (!message || !message->payload)
    {
        printf("LoRa: Received invalid message\n");
        return;
    }

    printf("LoRa: Processing message from address %d: '%s'\n",
           message->sender_address, message->payload);

    // Check for ON commands
    if (lora_is_on_command(message->payload))
    {
        stepper_active = true;

        // Clear any previous interrupt flag
        stepper_clear_interrupt();

        // Re-enable all motors after emergency stop
        if (global_steppers && global_num_steppers > 0)
        {
            for (uint i = 0; i < global_num_steppers; i++)
            {
                global_steppers[i].enabled = true;
            }
        }

        // Execute stepper sequence
        if (global_steppers && global_num_steppers > 0)
        {
            control_steppers(global_steppers, global_num_steppers);
        }

        // Send acknowledgment
        char ack_msg[] = "STEPPERS_ON";
        lora_send_message(&lora_config, message->sender_address, ack_msg, strlen(ack_msg));
    }
    // Check for OFF commands
    else if (lora_is_off_command(message->payload))
    {
        stepper_active = false;

        // Set interrupt flag to immediately stop any running stepper operation
        stepper_set_interrupt();

        // IMMEDIATELY disable all motors - NO DELAYS!
        if (global_steppers && global_num_steppers > 0)
        {
            stepper_emergency_stop_all(global_steppers, global_num_steppers);
        }

        // Send acknowledgment
        char ack_msg[] = "STEPPERS_OFF";
        lora_send_message(&lora_config, message->sender_address, ack_msg, strlen(ack_msg));
    }
    else
    {
        printf("LoRa: Unknown command: %s\n", message->payload);

        // Send error response
        char error_msg[] = "UNKNOWN_COMMAND";
        lora_send_message(&lora_config, message->sender_address, error_msg, strlen(error_msg));
    }
}

#ifdef LORA_TRANSMITTER_MODE
// Transmitter mode functions

void send_lora_command(const char *command)
{
    printf("Remote: Sending command '%s' to controller...\n", command);

    lora_status_t status = lora_send_message(&lora_config,
                                             STEPPER_CONTROLLER_ADDRESS,
                                             command,
                                             strlen(command));

    if (status == LORA_STATUS_OK)
    {
        printf("Remote: Command sent successfully\n");

        // NO LED feedback - LED only for signal reception
    }
    else
    {
        printf("Remote: Failed to send command (status: %d)\n", status);
    }
}

void run_transmitter_mode()
{
    // Give USB serial time to initialize
    sleep_ms(3000);

    printf("\n=== LoRa Remote Control Transmitter ===\n");
    printf("Remote: System starting up...\n");

    // Initialize buttons with internal pull-ups
    lora_buttons_init_all(buttons);

    // Initialize LoRa
    printf("Remote: Initializing LoRa module...\n");
    printf("Remote: UART1 TX=GPIO%d, RX=GPIO%d\n", LORA_TX_PIN, LORA_RX_PIN);
    printf("Remote: Network ID=%d, Address=%d, Frequency=%d MHz\n",
           LORA_NETWORK_ID, LORA_DEVICE_ADDRESS, LORA_FREQUENCY / 1000000);

    // Auto-detect LoRa module baud rate
    uint working_baud = detect_lora_baud_rate();

    lora_status_t status = lora_init_custom(&lora_config,
                                            LORA_UART_INST,
                                            LORA_TX_PIN,
                                            LORA_RX_PIN,
                                            LORA_NETWORK_ID,
                                            LORA_DEVICE_ADDRESS,
                                            LORA_FREQUENCY,
                                            LORA_POWER);

    if (status != LORA_STATUS_OK)
    {
        printf("Remote: LoRa initialization failed! Status code: %d\n", status);
        printf("Remote: Check wiring - VCC=3.3V, GND=GND, TXD=GPIO5, RXD=GPIO4\n");
        printf("Remote: LoRa module issue - check serial output!\n");
        while (1)
        {
            // NO LED blinking - LED only for signal reception
            sleep_ms(1000);
        }
    }

    printf("Remote: LoRa initialized successfully!\n");
    printf("Remote: Network ID: %d, Address: %d\n", LORA_NETWORK_ID, LORA_DEVICE_ADDRESS);
    printf("Remote: Target controller address: %d\n", STEPPER_CONTROLLER_ADDRESS);
    printf("\nRemote: Button Controls:\n");
    printf("  - Button 1 (GPIO 2): Send 'ON' command\n");
    printf("  - Button 2 (GPIO 3): Send 'OFF' command\n");
    printf("\nRemote: Ready for commands!\n");

    // Send startup announcement
    send_lora_command("REMOTE_READY");

    // Main transmitter loop
    while (1)
    {
        // Check for button presses
        if (lora_button_pressed(&buttons[0]))
        {
            send_lora_command("ON");
        }

        if (lora_button_pressed(&buttons[1]))
        {
            send_lora_command("OFF");
        }

        sleep_ms(10); // Small delay to prevent excessive CPU usage
    }
}
#endif

// Multi-baud-rate test function for LoRa module
bool test_lora_baud_rate(uint baud_rate)
{
    printf("Testing baud rate %d...\n", baud_rate);

    // Reinitialize UART with new baud rate
    uart_deinit(LORA_UART_INST);
    uart_init(LORA_UART_INST, baud_rate);
    gpio_set_function(LORA_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LORA_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(LORA_UART_INST, false, false);
    uart_set_format(LORA_UART_INST, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(LORA_UART_INST, true);

    sleep_ms(500); // Let UART stabilize

    // Clear any pending data
    while (uart_is_readable(LORA_UART_INST))
    {
        uart_getc(LORA_UART_INST);
    }

    // Send AT command
    uart_puts(LORA_UART_INST, "AT\r\n");
    sleep_ms(1000);

    if (uart_is_readable(LORA_UART_INST))
    {
        char buffer[32];
        int i = 0;
        while (uart_is_readable(LORA_UART_INST) && i < 31)
        {
            buffer[i] = uart_getc(LORA_UART_INST);
            i++;
        }
        buffer[i] = '\0';

        printf("Response at %d baud: '%s'\n", baud_rate, buffer);

        // Check if response contains "+OK" (good response)
        if (strstr(buffer, "+OK") != NULL)
        {
            printf("✅ Found working baud rate: %d\n", baud_rate);
            return true;
        }

        // Check if response has printable characters (partial success)
        bool has_printable = false;
        for (int j = 0; j < i; j++)
        {
            if (buffer[j] >= 32 && buffer[j] <= 126)
            {
                has_printable = true;
                break;
            }
        }

        if (has_printable)
        {
            printf("⚠️  Got readable response, but not +OK\n");
        }
        else
        {
            printf("❌ Got garbled response\n");
        }
    }
    else
    {
        printf("❌ No response at %d baud\n", baud_rate);
    }

    return false;
}

// Auto-detect LoRa module baud rate
uint detect_lora_baud_rate(void)
{
    printf("\n=== Auto-detecting LoRa module baud rate ===\n");
    printf("This will test common baud rates for RYLR998 module\n");

    // Common baud rates for RYLR998 modules
    uint baud_rates[] = {9600, 115200, 57600, 38400, 19200, 4800, 2400};
    uint num_rates = sizeof(baud_rates) / sizeof(baud_rates[0]);

    for (uint i = 0; i < num_rates; i++)
    {
        if (test_lora_baud_rate(baud_rates[i]))
        {
            printf("✅ Successfully detected baud rate: %d\n", baud_rates[i]);
            return baud_rates[i];
        }
        sleep_ms(200); // Small delay between tests
    }

    printf("❌ Could not detect working baud rate!\n");
    printf("Troubleshooting:\n");
    printf("1. Check power: LoRa module needs 3.3V (NOT 5V!)\n");
    printf("2. Check wiring: TXD->GPIO5, RXD->GPIO4, VCC->3.3V, GND->GND\n");
    printf("3. Try swapping TX/RX pins if still not working\n");
    printf("4. Check if module is getting power (LED should be on)\n");
    printf("5. Try a different LoRa module if available\n");

    return 9600; // Default fallback
}

void run(void)
{
    // Initialize LED pin configuration
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Add startup delay for serial output to stabilize
    sleep_ms(2000);

#ifdef LORA_TRANSMITTER_MODE
    printf("\n🔴 TRANSMITTER MODE ACTIVE 🔴\n");
    printf("This device is configured as REMOTE CONTROL\n");
    // Run in transmitter mode
    run_transmitter_mode();
#else
    printf("\n🔵 RECEIVER MODE ACTIVE 🔵\n");
    printf("This device is configured as STEPPER CONTROLLER\n");
    // Run in receiver mode (default)
    printf("\n=== LoRa Stepper Motor Controller ===\n");

    // Initialize 4 stepper motors with GPIO pins avoiding UART pins
    stepper_motor_t steppers[NUM_STEPPERS];

    if (!init_all_steppers(steppers, NUM_STEPPERS))
    {
        printf("Stepper motor initialization failed. Exiting...\n");
        return;
    }

    // Set global references for LoRa message handler
    global_steppers = steppers;
    global_num_steppers = NUM_STEPPERS;

    printf("All stepper motors initialized successfully!\n");

    // Initialize LoRa module
    printf("Initializing LoRa module...\n");

    // Auto-detect LoRa module baud rate
    uint working_baud = detect_lora_baud_rate();

    lora_status_t lora_status = lora_init_custom(&lora_config, LORA_UART_INST,
                                                 LORA_TX_PIN, LORA_RX_PIN,
                                                 LORA_NETWORK_ID, LORA_DEVICE_ADDRESS,
                                                 LORA_FREQUENCY, LORA_POWER);

    if (lora_status != LORA_STATUS_OK)
    {
        printf("LoRa initialization failed (status: %d). SAFETY MODE ACTIVE.\n", lora_status);
        printf("Safety: Steppers are DISABLED until LoRa is working properly\n");
        printf("Safety: Fix LoRa configuration issues before motors will activate\n");
    }
    else
    {
        printf("LoRa module initialized successfully!\n");
        printf("LoRa: Network ID: %d, Address: %d, Freq: %ld Hz\n",
               LORA_NETWORK_ID, LORA_DEVICE_ADDRESS, LORA_FREQUENCY);
        printf("LoRa: Steppers will ONLY run when commanded via LoRa\n");

        // Verify LoRa configuration by querying the module
        printf("LoRa: Verifying module configuration...\n");
        char response[64];

        // Check network ID
        lora_status_t status = lora_send_at_command(&lora_config, "AT+NETWORKID?", response, sizeof(response));
        if (status == LORA_STATUS_OK)
        {
            printf("LoRa: Current Network ID: %s\n", response);
        }
        else
        {
            printf("LoRa: ❌ Failed to get Network ID (status: %d)\n", status);
        }

        // Check device address
        status = lora_send_at_command(&lora_config, "AT+ADDRESS?", response, sizeof(response));
        if (status == LORA_STATUS_OK)
        {
            printf("LoRa: Current Address: %s\n", response);
        }
        else
        {
            printf("LoRa: ❌ Failed to get Address (status: %d)\n", status);
        }

        // Check frequency
        status = lora_send_at_command(&lora_config, "AT+BAND?", response, sizeof(response));
        if (status == LORA_STATUS_OK)
        {
            printf("LoRa: Current Frequency: %s MHz\n", response);
        }
        else
        {
            printf("LoRa: ❌ Failed to get Frequency (status: %d)\n", status);
        }

        // Set up LoRa message handler
        lora_set_message_handler(&lora_config, lora_message_handler, steppers);

        // Send startup message to announce receiver is ready
        char startup_msg[] = "STEPPER_CONTROLLER_READY";
        lora_broadcast_message(&lora_config, startup_msg, strlen(startup_msg));
    }

    printf("Starting LED blink, stepper motor control, and LoRa communication loop...\n");
    printf("LoRa Commands: ON/START/MOVE/1 to activate, OFF/STOP/HALT/0 to deactivate\n");

    uint cycle_count = 0;
    bool lora_initialized = (lora_status == LORA_STATUS_OK);

    // Main receiver loop
    while (true)
    {
        // NO LED blinking - LED only for signal reception

        // Process LoRa messages if initialized
        if (lora_initialized)
        {
            lora_status_t msg_status = lora_process_messages(&lora_config);
            if (msg_status == LORA_STATUS_OK)
            {
                // ONLY LED usage: Flash when receiving a LoRa signal
                gpio_put(PICO_DEFAULT_LED_PIN, 1);
                // Immediate turn-off - no delay
                gpio_put(PICO_DEFAULT_LED_PIN, 0);
            }

            // Run steppers continuously when active
            if (stepper_active)
            {
                // Check if steppers were interrupted
                if (stepper_is_interrupted())
                {
                    stepper_active = false;
                }
                else
                {
                    control_steppers(steppers, NUM_STEPPERS);

                    // Check again after stepper operation in case it was interrupted
                    if (stepper_is_interrupted())
                    {
                        stepper_active = false;
                    }
                }
            }
        }
        else
        {
            // Safety mode: NO motor activation without proper LoRa
            // Remove periodic messaging to maximize LoRa processing speed
        }

        cycle_count++;

        // Remove periodic status updates to maximize LoRa processing speed
        // Status updates would add delays to the main loop
    }
#endif
}
