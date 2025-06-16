# RYLR998 LoRa Module Wiring Guide

## Overview
This project integrates a RYLR998 LoRa transceiver module with a Raspberry Pi Pico to enable wireless remote control of stepper motors. The LoRa module communicates via UART1 using AT commands.

## GPIO Pin Assignments

### RYLR998 LoRa Module
- **VCC**: 3.3V (Pin 36)
- **GND**: GND (Pin 38)
- **TXD**: GPIO 5 (Pin 7) - Connected to Pico UART1 RX
- **RXD**: GPIO 4 (Pin 6) - Connected to Pico UART1 TX
- **RST**: Not connected (optional)
- **ANT**: External antenna connection (optional for better range)

### UART Configuration
- **UART Instance**: UART1
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

## Power Supply Notes
- The RYLR998 operates at 3.3V (DO NOT use 5V)
- Current consumption: ~12mA in receive mode, ~120mA in transmit mode
- Connect VCC to 3.3V output pin (pin 36) on Raspberry Pi Pico
- Ensure stable power supply for reliable operation

## LoRa Configuration
- **Frequency**: 915 MHz (US ISM band)
- **Network ID**: 1 (configurable)
- **Device Address**: 100 (configurable)
- **Transmission Power**: 10 (0-15 range)
- **Spreading Factor**: 12 (maximum range)
- **Bandwidth**: 125 kHz
- **Coding Rate**: 4/5

## Supported Commands
The system responds to the following LoRa commands (case insensitive):

### Motor Activation Commands
- `ON` - Activate stepper motor sequence
- `START` - Activate stepper motor sequence
- `MOVE` - Activate stepper motor sequence
- `1` - Activate stepper motor sequence

### Motor Deactivation Commands
- `OFF` - Deactivate stepper motor operation
- `STOP` - Stop stepper motor operation
- `HALT` - Stop stepper motor operation
- `0` - Deactivate stepper motor operation

## Operation
The program will:
1. Initialize LoRa module on startup
2. Send "STEPPER_CONTROLLER_READY" broadcast message
3. Continuously monitor for incoming LoRa messages
4. Execute stepper motor sequence when receiving ON commands
5. Send acknowledgment messages back to sender
6. Blink onboard LED to indicate system activity
7. Send periodic status updates every 30 seconds

## Connection Diagram
```
RYLR998 LoRa Module         Raspberry Pi Pico
VCC (3.3V)    -----------> Pin 36 (3V3 OUT)
GND           -----------> Pin 38 (GND)
TXD           -----------> Pin 7 (GPIO 5 - UART1 RX)
RXD           -----------> Pin 6 (GPIO 4 - UART1 TX)
ANT           -----------> External antenna (optional)
```

## Range and Performance
- **Indoor Range**: Up to 500m with obstacles
- **Outdoor Range**: Up to 3km line of sight
- **Data Rate**: Variable based on spreading factor
- **Frequency Band**: 915 MHz (US ISM band)
- **Modulation**: LoRa (Long Range)

## AT Command Examples
The RYLR998 uses standard AT commands for configuration:

```
AT                    # Test communication
AT+ADDRESS=100        # Set device address to 100
AT+NETWORKID=1        # Set network ID to 1
AT+BAND=915000000     # Set frequency to 915 MHz (US)
AT+POWER=10           # Set transmission power to 10
AT+SEND=200,5,HELLO   # Send "HELLO" to device address 200
```

## Message Format
Received messages follow this format:
```
+RCV=<sender_address>,<length>,<payload>,<rssi>
Example: +RCV=200,2,ON,-45
```

## Troubleshooting
- **No LoRa communication**: Check wiring, ensure 3.3V power, verify UART pins
- **Module not responding**: Check AT commands, ensure correct baud rate (115200)
- **Short range**: Add external antenna, check frequency configuration
- **Commands not working**: Verify network ID and addressing configuration

## Files Added/Modified
- `src/lora.h` - LoRa module driver header with RYLR998 interface
- `src/lora.c` - LoRa module driver implementation with AT commands
- `src/run.c` - Modified to include LoRa communication and message handling
- `src/run.h` - Updated with LoRa function declarations
- `CMakeLists.txt` - Updated to compile lora.c and link hardware_uart

## Testing
1. Flash the firmware to Raspberry Pi Pico
2. Connect LoRa module as per wiring diagram
3. Use another RYLR998 module or LoRa device to send test commands
4. Monitor serial output for LoRa communication status
5. Send "ON" command to trigger stepper motor sequence
6. Verify acknowledgment messages are received
