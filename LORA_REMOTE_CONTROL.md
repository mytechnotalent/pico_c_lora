# Unified LoRa Project with Remote Control

## Overview
This project now supports **both transmitter and receiver modes** using the same codebase! You can build either a LoRa remote control transmitter or a stepper motor controller receiver using compile-time flags.

## Key Features

### **✅ No External Resistors Required**
- Uses **internal pull-up resistors** in the Pico
- Simply connect buttons between GPIO and GND
- No external 10kΩ resistors needed!

### **✅ Single Codebase**
- Same `src/` files for both transmitter and receiver
- Compile-time flags (`LORA_TRANSMITTER_MODE` / `LORA_RECEIVER_MODE`)
- Clean separation using `#ifdef` preprocessor directives

### **✅ Easy Building**
```bash
# Build receiver mode (stepper controller)
cmake -G "Unix Makefiles" ..
make

# Build transmitter mode (remote control)
cmake -G "Unix Makefiles" -DBUILD_TRANSMITTER=ON ..
make
```

## Hardware Requirements

### For Remote Control Transmitter
- **Raspberry Pi Pico** (regular Pico, no WiFi needed)
- **RYLR998 LoRa Module**
- **2 Push buttons** (momentary, normally open)
- **Breadboard and jumper wires**
- **Power source** (USB cable, battery pack, or 3xAA batteries)

### For Stepper Controller Receiver  
- **Raspberry Pi Pico**
- **RYLR998 LoRa Module**
- **4x ULN2003 stepper motor drivers**
- **4x 28BYJ-48 stepper motors**
- **5V power supply** (for motors)

## Wiring Diagrams

### LoRa Module (Both Devices)
```
RYLR998 Module → Raspberry Pi Pico
VCC     → 3.3V (Pin 36)
GND     → GND (Pin 38)
TXD     → GPIO 5 (Pin 7) - UART1 RX
RXD     → GPIO 4 (Pin 6) - UART1 TX
```

### Buttons (Transmitter Only)
```
Button 1 (ON)   → GPIO 2 (Pin 4)  ──┐ 
Button 2 (OFF)  → GPIO 3 (Pin 5)  ──┘ Both connect to GND when pressed

Internal pull-ups automatically enabled - NO external resistors needed!
```

## Building the Project

### Method 1: Using CMake Directly

**Receiver Mode (Stepper Controller):**
```bash
cd /Users/kevinthomas/Desktop/pico_c_lora
rm -rf build && mkdir build && cd build
cmake -G "Unix Makefiles" ..
make
# Output: lora_receiver.uf2
```

**Transmitter Mode (Remote Control):**
```bash
cd /Users/kevinthomas/Desktop/pico_c_lora
rm -rf build && mkdir build && cd build  
cmake -G "Unix Makefiles" -DBUILD_TRANSMITTER=ON ..
make
# Output: lora_transmitter.uf2
```

### Method 2: Using VS Code Tasks

The VS Code tasks automatically build in receiver mode. To build transmitter mode, modify the CMakeLists.txt temporarily or use the terminal method above.

## 🚀 Easy Build Scripts

Two convenient shell scripts are provided for error-free building:

### **Build Transmitter (Remote Control)**
```bash
./build_transmitter.sh
```
- Cleans previous build
- Configures for transmitter mode
- Builds firmware
- Shows clear instructions

### **Build Receiver (Stepper Controller)**
```bash
./build_receiver.sh
```
- Cleans previous build  
- Configures for receiver mode (default)
- Builds firmware
- Shows clear instructions

### **Manual Building (Alternative)**
If you prefer manual control:

**Transmitter Mode:**
```bash
rm -rf build && mkdir build && cd build
cmake -G "Unix Makefiles" -DBUILD_TRANSMITTER=ON ..
make
```

**Receiver Mode:**
```bash
rm -rf build && mkdir build && cd build
cmake -G "Unix Makefiles" ..
make
```

---
## Network Configuration

### Device Addresses
- **Transmitter**: Address 200
- **Receiver**: Address 100  
- **Network ID**: 1 (both devices)
- **Frequency**: 915 MHz (US ISM band)

### Commands Supported
**Transmitter sends these commands:**
- `"ON"` - Activate stepper motors
- `"OFF"` - Deactivate stepper motors  
- `"REMOTE_READY"` - Startup announcement

**Receiver acknowledges with:**
- `"STEPPERS_ON"` - Motors activated
- `"STEPPERS_OFF"` - Motors deactivated
- `"STEPPER_CONTROLLER_READY"` - Startup ready
- `"STATUS_OK"` - Periodic status (every 30 seconds)

## Operation Guide

### 1. Build Both Devices
1. **Build receiver firmware** using default cmake
2. **Build transmitter firmware** using `-DBUILD_TRANSMITTER=ON`
3. Flash receiver firmware to stepper controller Pico
4. Flash transmitter firmware to remote control Pico

### 2. Setup Hardware
1. **Wire LoRa modules** to both Picos (same wiring)
2. **Wire stepper motors** to receiver Pico only
3. **Wire buttons** to transmitter Pico only
4. **Power both devices**

### 3. Test Communication
1. **Power on receiver first** - watch for "STEPPER_CONTROLLER_READY"
2. **Power on transmitter** - watch for "REMOTE_READY" 
3. **Press ON button** - should activate stepper motors
4. **Press OFF button** - should deactivate stepper motors

## Button Functions (Transmitter)

| Button   | GPIO   | Function          | Command Sent |
| -------- | ------ | ----------------- | ------------ |
| Button 1 | GPIO 2 | Activate Motors   | `"ON"`       |
| Button 2 | GPIO 3 | Deactivate Motors | `"OFF"`      |

**Visual Feedback:**
- **Onboard LED blinks** when command is transmitted
- **Serial output** shows command status
- **Range**: Up to 3km outdoor, 100-500m indoor

## Code Structure

### Compile-Time Flags
```c
// In lora.h
#ifdef LORA_TRANSMITTER_MODE
    // Transmitter-specific code
#else  
    // Receiver-specific code (default)
#endif
```

### Key Functions
**Shared (Both Modes):**
- `lora_init_custom()` - Initialize LoRa module
- `lora_send_message()` - Send LoRa commands
- `lora_process_messages()` - Handle incoming messages

**Transmitter Mode:**
- `lora_button_init()` - Setup button with internal pull-up
- `lora_button_pressed()` - Check button state with debouncing
- `send_lora_command()` - Send command and blink LED
- `run_transmitter_mode()` - Main transmitter loop

**Receiver Mode:**
- `lora_message_handler()` - Process received commands
- `control_steppers()` - Execute stepper motor sequence
- `init_all_steppers()` - Initialize stepper motors

## Troubleshooting

### Build Issues
- **Wrong mode building**: Check CMake flags (`-DBUILD_TRANSMITTER=ON`)
- **Compilation errors**: Ensure all files updated and clean build

### Communication Issues  
- **No LoRa communication**: Check 3.3V power, wiring, and frequency (915 MHz)
- **Commands not working**: Verify network ID (1) and addresses (200↔100)

### Button Issues
- **Buttons not responding**: Check wiring to GND, GPIO pins, and debouncing
- **Multiple triggers**: Increase debounce time in `lora_button_pressed()`

### Range Issues
- **Short range**: Check antenna connections, reduce obstacles
- **Interference**: Try different frequency or location

### ⚠️ TROUBLESHOOTING: Fast Blinking LED

If your **transmitter Pico** has the **onboard LED blinking rapidly** (fast on/off), this indicates **LoRa initialization failure**. Here's how to fix it:

### **Step 1: Check LoRa Module Wiring**
The most common cause is incorrect LoRa module wiring:

```
CRITICAL WIRING CHECK:
RYLR998 → Pico (Transmitter)
VCC → Pin 36 (3.3V OUT) ⚠️ NOT 5V!
GND → Pin 38 (GND)
TXD → Pin 7 (GPIO 5 - UART1 RX)
RXD → Pin 6 (GPIO 4 - UART1 TX)
```

**⚠️ COMMON MISTAKES:**
- **Wrong Voltage**: RYLR998 needs **3.3V**, not 5V (will damage module!)
- **Swapped TX/RX**: Module TXD goes to Pico RX (GPIO 5)
- **Loose Connections**: Ensure solid breadboard connections
- **No Antenna**: Module needs antenna for proper operation

### **Step 2: Serial Debug Output**
1. Connect transmitter Pico to computer via USB
2. Open serial terminal (9600 baud, 8N1)
3. Look for error messages:
   - `"Remote: LoRa initialization failed! Check wiring."`
   - `"Remote: LoRa initialized successfully!"` (this means it's working)

### **Step 3: Test LoRa Module**
Use a serial terminal to test the LoRa module directly:
```bash
# Connect to Pico's UART (if accessible)
# Send AT commands to test module:
AT          # Should respond "OK"
AT+VER      # Should show firmware version
```

### **Step 4: Power Supply Check**
- **USB Power**: Use good quality USB cable
- **3.3V Rail**: Measure voltage at Pin 36 (should be 3.2-3.4V)
- **Current**: LoRa module draws ~120mA when transmitting

### **Step 5: Range Test**
- Start with devices **very close** (1-2 feet apart)
- Both should show successful initialization
- Test button presses at close range first

## 🚨 URGENT: Fast Blinking LED Troubleshooting

### **Problem**: Transmitter LED Blinks Fast (200ms) + Steppers Don't Respond

**Root Cause**: LoRa initialization is failing on the transmitter Pico.

### **Critical Settings Check**

Both RYLR998 modules MUST have identical settings:

#### **Required Settings for Both Modules:**
```
Baud Rate: 9600 (NOT 115200!)
Frequency: 915000000 (915 MHz)
Network ID: 1
Spreading Factor: 12
Bandwidth: 125 kHz
Coding Rate: 4/5
```

#### **Device Addresses:**
- **Transmitter**: Address 200
- **Receiver**: Address 100

### **Step-by-Step Diagnosis**

#### **1. Check Serial Output**
Connect to transmitter via USB and monitor serial output:
```
Expected: "LoRa: Module initialized successfully"
Actual: "LoRa: Initial communication test failed"
```

#### **2. Verify Wiring**
**CRITICAL**: RYLR998 modules are **3.3V ONLY**!
```
RYLR998 → Pico
VCC → 3.3V (Pin 36) ← NOT 5V!
GND → GND (Pin 38)
TXD → GPIO 5 (Pin 7)
RXD → GPIO 4 (Pin 6)
```

#### **3. Test LoRa Module Manually**
Use a USB-to-serial adapter to test the RYLR998:
```bash
# Connect at 9600 baud
AT                    # Should respond: +OK
AT+BAND=915000000     # Should respond: +OK
AT+NETWORKID=1        # Should respond: +OK
AT+ADDRESS=200        # Should respond: +OK (transmitter)
AT+ADDRESS=100        # Should respond: +OK (receiver)
```

#### **4. Common Issues & Fixes**

**Issue**: No response to AT commands
**Fix**: Check 3.3V power, wiring, and baud rate (9600)

**Issue**: Wrong baud rate detected
**Fix**: RYLR998 defaults to 9600, not 115200

**Issue**: Modules on different frequencies
**Fix**: Both must be exactly 915000000 Hz

**Issue**: Different network IDs
**Fix**: Both must use Network ID = 1

**Issue**: Antenna problems
**Fix**: Ensure antenna connections are secure

### **Quick Fix Commands**

If you can connect to the LoRa module manually:
```
AT+BAND=915000000
AT+NETWORKID=1
AT+ADDRESS=200
AT+POWER=10
AT+PARAMETER=12,4,1,4
```

### **Power Supply Check**
- Measure 3.3V at RYLR998 VCC pin
- Ensure stable power (no voltage drops)
- USB power should be sufficient

### **Range Test**
Start with modules **1 foot apart** for initial testing!

---
## Advantages of This Approach

✅ **Single Project**: One codebase for both devices  
✅ **No External Resistors**: Internal pull-ups simplify wiring  
✅ **Flexible Building**: Easy to switch between modes  
✅ **Clean Code**: Proper separation with compile flags  
✅ **US Compliant**: 915 MHz frequency for legal operation  
✅ **Professional**: Well-documented and maintainable code  
✅ **Simplified Controls**: Only two buttons needed for essential functions

This unified approach makes it much easier to maintain and understand the project while providing a clean separation between transmitter and receiver functionality!

### **🚨 CRITICAL: No LED + Blank Serial Monitor**

This indicates a **fundamental system problem**:

#### **Possible Causes:**
1. **Wrong firmware flashed** - Not in transmitter mode
2. **System crash** - Code is hanging somewhere
3. **USB serial not working** - Connection issue
4. **Power supply problem** - Insufficient power

#### **Quick Diagnostic Steps:**

**1. LED Status Check:**
- **No LED at all** = System not running
- **Solid LED on** = System frozen
- **Fast blinking** = LoRa init failed (previous issue)
- **Normal operation** = Should see slow blinks

**2. USB Serial Connection:**
- **Unplug and replug** USB cable
- **Try different USB port**
- **Check baud rate**: 115200, 8N1
- **Try different terminal program**

**3. Power Supply:**
- **Measure 3.3V** at Pico pin 36
- **Check USB cable** quality
- **Try powered USB hub**

**4. Firmware Verification:**
```bash
# Rebuild and reflash transmitter mode
cd build
rm -rf *
cmake -G "Unix Makefiles" -DBUILD_TRANSMITTER=ON ..
make
# Flash stepper.uf2 to Pico
```

**5. Basic Test:**
Remove LoRa module temporarily and test just the Pico:
- Should see startup messages
- LED should blink normally

#### **Expected Serial Output:**
```
=== LoRa Remote Control Transmitter ===
Remote: System starting up...
Button: 2 buttons initialized with internal pull-ups
Remote: Initializing LoRa module...
```

If you see **nothing**, the issue is **before LoRa** - likely power or firmware.
