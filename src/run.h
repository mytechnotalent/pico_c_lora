/**
 * @file run.h
 * @brief LED control, stepper motor, and LoRa communication application interface
 * @version 1.0
 * @date 2025-06-15
 * @author Kevin Thomas
 *
 * This header file contains the main application interface for controlling
 * the onboard LED, ULN2003 28BYJ-48 stepper motors, and RYLR998 LoRa module
 * on the Raspberry Pi Pico. The module provides a unified interface for the
 * combined LED, stepper motor control, and wireless communication functionality.
 *
 * Hardware Requirements:
 * - Raspberry Pi Pico board
 * - Onboard LED (GPIO 25)
 * - 4x ULN2003 driver boards with 28BYJ-48 stepper motors
 * - RYLR998 LoRa module connected via UART1
 * - 5V power supply via USB (VBUS)
 *
 * GPIO Pin Assignments:
 * - Stepper 1: GPIO 2,3,6,7
 * - Stepper 2: GPIO 10,11,14,15
 * - Stepper 3: GPIO 18,19,20,21
 * - Stepper 4: GPIO 22,26,27,28
 * - LED: GPIO 25 (onboard)
 * - LoRa UART1: GPIO 4 (TX), GPIO 5 (RX)
 *
 * @copyright Copyright (c) 2025 Kevin Thomas
 */

#ifndef RUN_H
#define RUN_H

#include "pico/stdlib.h"
#include "stepper.h"
#include "lora.h"

/**
 * @brief Main application function
 *
 * This function serves as the main application entry point that initializes
 * and controls both the LED blinking and stepper motor operations. It performs
 * the following sequence:
 *
 * 1. Initialize LED GPIO configuration
 * 2. Initialize 4 stepper motors with predefined GPIO pins
 * 3. Enter infinite loop alternating between LED control and stepper operations
 *
 * The function runs an infinite loop where:
 * - LED blinks continuously with 1-second cycles
 * - Every 5 LED cycles, all 4 stepper motors perform a demonstration sequence
 * - Each stepper rotates 45° clockwise, then 45° counter-clockwise
 * - Serial output provides status information for debugging
 *
 * GPIO Pin Configuration:
 * - Avoids UART pins (0,1,4,5,8,9,12,13,16,17) to prevent communication conflicts
 * - Uses 5V power from VBUS for stepper motor drivers
 * - 3.3V GPIO signals provide logic control to ULN2003 drivers
 *
 * @note This function never returns under normal operation as it contains
 *       an infinite loop. The program continues until power removal or reset.
 *
 * @pre stdio_init_all() must be called before this function
 * @post System enters infinite operational loop
 *
 * @return void
 *
 * @see stepper_init() - Initialize stepper motor configuration
 * @see stepper_rotate_degrees() - Perform stepper motor rotation
 * @see gpio_init() - Initialize GPIO pins
 * @see gpio_set_dir() - Set GPIO pin direction
 * @see gpio_put() - Control GPIO pin state
 */
void run(void);

/**
 * @brief Execute one LED blink cycle
 *
 * Performs a complete LED blink cycle consisting of turning the LED on,
 * waiting 500ms, turning it off, and waiting another 500ms. Provides
 * serial output for status monitoring and debugging purposes.
 *
 * The function controls the onboard LED (typically GPIO 25) and outputs
 * status messages via serial communication for monitoring the LED state.
 *
 * @note The LED pin must be initialized as an output before calling this function
 * @warning This function blocks execution for exactly 1000ms per call
 *
 * @pre LED GPIO pin must be initialized and configured as output
 * @pre stdio_init_all() must be called for serial output functionality
 * @post LED state will be OFF upon function exit
 *
 * @param led_pin The GPIO pin number for the LED to control
 * @return void
 *
 * @see gpio_put() - Set GPIO pin high/low state
 * @see sleep_ms() - Blocking millisecond delay function
 * @see printf() - Standard output for status messages
 */
void led_blink_cycle(uint led_pin);

/**
 * @brief Control stepper motors in demonstration sequence
 *
 * Executes a demonstration sequence for all stepper motors, rotating each
 * motor in both directions to verify proper operation. The sequence consists
 * of clockwise rotation followed by counter-clockwise rotation for each motor.
 *
 * Operation sequence per motor:
 * 1. Rotate 45° clockwise with status output
 * 2. Wait 500ms
 * 3. Rotate 45° counter-clockwise with status output
 * 4. Wait 500ms before proceeding to next motor
 *
 * @note All stepper motors must be properly initialized before calling
 * @warning This function has significant execution time (several seconds)
 *
 * @pre All stepper motors in array must be initialized with stepper_init()
 * @pre stdio_init_all() must be called for serial output functionality
 * @post All stepper motors return to their starting positions
 *
 * @param steppers Pointer to array of stepper motor structures
 * @param num_steppers Number of stepper motors in the array (must be ≤ MAX_STEPPERS)
 * @return void
 *
 * @see stepper_rotate_degrees() - Perform individual motor rotation
 * @see stepper_direction_t - Motor rotation direction enumeration
 * @see sleep_ms() - Inter-operation timing delays
 */
void control_steppers(stepper_motor_t *steppers, uint num_steppers);

/**
 * @brief Handle incoming LoRa messages for stepper motor control
 *
 * This callback function processes incoming LoRa messages and controls
 * stepper motors based on the received commands. Supports ON/START/MOVE/1
 * commands to activate stepper motors and OFF/STOP/HALT/0 commands to
 * deactivate them.
 *
 * Supported commands (case insensitive):
 * - ON, START, MOVE, 1: Activate stepper motor sequence
 * - OFF, STOP, HALT, 0: Stop stepper motor operation
 *
 * @note This function is called automatically when LoRa messages are received
 * @warning Stepper operations are blocking and may affect LoRa response timing
 *
 * @pre LoRa module must be initialized and message handler set
 * @pre Stepper motors passed via user_data must be initialized
 * @post Stepper motors will execute the requested operation
 *
 * @param message Pointer to received LoRa message structure
 * @param user_data User data pointer containing stepper motor array
 * @return void
 *
 * @see lora_message_t - LoRa message structure definition
 * @see lora_is_on_command() - Check for activation commands
 * @see lora_is_off_command() - Check for deactivation commands
 */
void lora_message_handler(const lora_message_t *message, void *user_data);

/**
 * @brief Test LoRa module communication at specific baud rate
 *
 * Tests communication with the LoRa module at a specific baud rate by
 * sending an AT command and checking for a valid response.
 *
 * @param baud_rate The baud rate to test (e.g., 9600, 115200)
 * @return true if communication successful, false otherwise
 */
bool test_lora_baud_rate(uint baud_rate);

/**
 * @brief Auto-detect LoRa module baud rate
 *
 * Automatically detects the correct baud rate for the LoRa module by
 * testing common baud rates and checking for valid responses.
 *
 * @return The detected working baud rate, or 9600 as fallback
 */
uint detect_lora_baud_rate(void);

#ifdef LORA_TRANSMITTER_MODE
/**
 * @brief Send LoRa command to stepper controller
 *
 * Transmits a command string to the stepper motor controller via LoRa
 * and provides visual feedback through the onboard LED.
 *
 * @param command Command string to send (e.g., "ON", "OFF", "TEST")
 * @return void
 */
void send_lora_command(const char *command);

/**
 * @brief Run transmitter mode main loop
 *
 * Initializes buttons and LoRa module for transmitter operation, then
 * enters the main loop to monitor button presses and send commands.
 *
 * @return void
 */
void run_transmitter_mode(void);
#endif

#endif /* RUN_H */
