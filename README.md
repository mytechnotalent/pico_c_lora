![image](https://github.com/mytechnotalent/pico_c_lora/blob/main/LoRa.jpeg?raw=true)

# Pico C LoRa

A professional embedded C application for the Raspberry Pi Pico that combines 4-channel LoRa motor control with RYLR998 LoRa wireless communication. The project supports both receiver (LoRa controller) and transmitter (remote control) modes in a unified codebase, enabling wireless LoRa motor control over long distances.

<br>

## FREE Reverse Engineering Self-Study Course [HERE](https://github.com/mytechnotalent/Reverse-Engineering-Tutorial)

<br>

## Wiring
![image](https://github.com/mytechnotalent/pico_c_lora/blob/main/diagrams/Debug-Probe-Wiring.png?raw=true)
![image](https://github.com/mytechnotalent/pico_c_lora/blob/main/diagrams/lora_transmitter.png?raw=true)
![image](https://github.com/mytechnotalent/pico_c_lora/blob/main/diagrams/lora_receiver.png?raw=true)

## Features

- **LoRa Wireless Control**: Long-range communication using RYLR998 modules (up to 3km)
- **Unified Codebase**: Single project for both transmitter and receiver modes
- **2-Button Remote**: Simple ON/OFF control with internal pull-ups (no external resistors)
- **4 LoRa Motors**: Individual control of ULN2003 28BYJ-48 LoRa motors
- **LED Control**: Onboard LED blinking with visual feedback
- **UART-Safe GPIO**: Avoids UART pins to prevent communication conflicts
- **Professional Code Structure**: Modular design with comprehensive documentation
- **5V Power Support**: Utilizes VBUS for optimal LoRa motor performance

## Hardware Requirements

### For LoRa Controller (Receiver Mode)
- Raspberry Pi Pico development board
- RYLR998 LoRa module (915 MHz)
- 4× ULN2003 LoRa motor driver boards  
- 4× 28BYJ-48 LoRa motors (5V, 4-phase)
- USB cable for power and programming
- Breadboard and jumper wires

### For Remote Control (Transmitter Mode)
- Raspberry Pi Pico development board
- RYLR998 LoRa module (915 MHz)
- 2× Push buttons (momentary, normally open)
- Breadboard and jumper wires
- Power source (USB, battery pack, or 3×AA batteries)

### Power Specifications
- **Logic Power**: 3.3V (from Pico internal regulator)
- **Motor Power**: 5V (from USB VBUS pin)
- **Current per Motor**: ~160mA
- **Total Current**: 640mA (well within USB 900mA limit)

## GPIO Pin Assignments

| Component           | GPIO Pins      | Description        |
| ------------------- | -------------- | ------------------ |
| **LoRa Module**     | 4, 5           | UART1 TX, RX       |
| **Remote Buttons**  | 2, 3           | ON, OFF (TX mode)  |
| **LoRa Motor 1** | 2, 3, 6, 7     | IN1, IN2, IN3, IN4 |
| **LoRa Motor 2** | 10, 11, 14, 15 | IN1, IN2, IN3, IN4 |
| **LoRa Motor 3** | 18, 19, 20, 21 | IN1, IN2, IN3, IN4 |
| **LoRa Motor 4** | 22, 26, 27, 28 | IN1, IN2, IN3, IN4 |
| **Onboard LED**     | 25             | Built-in LED       |

### Avoided UART Pins
GPIO pins 0, 1, 8, 9, 12, 13, 16, 17 are intentionally avoided to prevent conflicts with UART communication. GPIO 4 and 5 are used specifically for LoRa UART1.

## Wiring Connections

### ULN2003 Driver Connections
```
Pico → ULN2003 (per motor)
GPIO → IN1, IN2, IN3, IN4
3.3V → VCC (logic power)
GND  → GND

ULN2003 → 28BYJ-48 Motor
OUT1 → Blue wire
OUT2 → Pink wire  
OUT3 → Yellow wire
OUT4 → Orange wire
VCC  → Red wire (5V from Pico VBUS)
```

### Power Distribution
```
USB 5V → Pico VBUS → Motor power (red wires)
Pico 3.3V → ULN2003 VCC → Logic power
Common GND for all components
```

For detailed wiring diagrams, see [LoRa_WIRING.md](LoRa_WIRING.md).

## Building the Project

### Prerequisites
- Raspberry Pi Pico SDK installed
- CMake 3.13 or higher
- ARM GCC toolchain
- VS Code with Pico extension (recommended)

### Build Commands
```bash
mkdir build
cd build
cmake ..
make
```

### Output Files
- `LoRa.uf2` - Main firmware file for drag-and-drop programming
- `LoRa.elf` - ELF executable for debugging
- `LoRa.bin` - Raw binary file
- `LoRa.hex` - Intel HEX format

## Programming the Pico

1. Hold BOOTSEL button while connecting USB
2. Drag `LoRa.uf2` to the RPI-RP2 drive
3. Pico will automatically reboot and start the application

## Operation

### Program Behavior
1. **Initialization**: LED and LoRa motors configured
2. **LED Blinking**: Continuous 1-second cycles with serial output
3. **LoRa Demo**: Every 5 LED cycles, all motors demonstrate movement
4. **Serial Output**: Status messages via USB serial (115200 baud)

### Serial Output Example
```
All LoRa motors initialized successfully!
Starting LED blink and LoRa motor control loop...
LED ON
LED OFF
LED ON
LED OFF
[... continues for 5 cycles ...]
Running LoRa motor demonstration sequence...
Moving LoRa motor 1 clockwise 45 degrees
Moving LoRa motor 1 counter-clockwise 45 degrees
[... continues for all 4 motors ...]
LoRa sequence complete
```

### Performance Characteristics
- **Step Timing**: 3ms delay between steps (configurable)
- **LED Cycle**: 1 second (500ms on, 500ms off)
- **LoRa Demo**: Every 5 LED cycles (45° CW, then 45° CCW per motor)
- **Serial Output**: Status messages for debugging

## API Reference

See header files for complete API documentation:
- `src/run.h` - Main application interface
- `src/LoRa.h` - LoRa motor driver interface

## Reverse Engineering & Analysis

This project includes a comprehensive reverse engineering data generation script for educational purposes and deep analysis of the compiled binary.

### Generating Analysis Data

```bash
# Generate complete reverse engineering dataset
./generate_reverse_engineering_data.sh
```

The script creates a `data/` folder containing comprehensive analysis files **and generates a professional PDF report**: 

#### 📚 **"Hacking Embedded LoRa" by Kevin Thomas**
- **Complete PDF Guide** - Professional reverse engineering documentation
- **Cover Artwork** - Uses `LoRa.jpeg` as the front cover
- **8 Comprehensive Chapters** - From basic analysis to advanced topics
- **Educational Focus** - Perfect for learning embedded systems reverse engineering

**Requirements for PDF generation:**
```bash
# Install pandoc (if not already installed)
brew install pandoc              # macOS
sudo apt install pandoc         # Ubuntu/Debian
```

### Binary Analysis Results

#### Memory Layout
The compiled binary has the following memory organization:

```
Flash Memory (2MB total):
├── .boot2      (0x10000000): 256 bytes   - RP2040 bootloader
├── .text       (0x10000100): 16,512 bytes - Program code
├── .rodata     (0x10004180): 1,284 bytes  - Read-only data
└── .binary_info(0x10004684): 32 bytes    - Binary metadata

SRAM (264KB total):
├── .ram_vector_table: 192 bytes  - Interrupt vector table
├── .data      : 296 bytes        - Initialized variables
├── .bss       : 1,000 bytes      - Uninitialized variables
├── .heap      : 2,048 bytes      - Dynamic memory
└── .stack     : 2,048 bytes      - Function call stack
```

#### Key Functions Analysis

**Main Function Disassembly:**
```assembly
100002d4 <main>:
100002d4: b510         push    {r4, lr}         ; Save registers
100002d6: f003 fe07    bl      0x10003ee8 <stdio_init_all>  ; Initialize UART
100002da: f000 f803    bl      0x100002e4 <run>             ; Call main loop
100002de: 2000         movs    r0, #0x0                     ; Return 0
100002e0: bd10         pop     {r4, pc}                     ; Restore & return
```

**Run Function (Main Loop):**
```assembly
100002e4 <run>:
100002e4: b5f0         push    {r4, r5, r6, r7, lr}        ; Save registers
100002e6: 46de         mov     lr, r11                      ; High register save
100002e8: 4657         mov     r7, r10
100002ea: 464e         mov     r6, r9
100002ec: 4645         mov     r5, r8
100002ee: b5e0         push    {r5, r6, r7, lr}            ; Push high regs
100002f0: 2019         movs    r0, #0x19                   ; GPIO 25 (LED)
100002f2: b0a5         sub     sp, #0x94                   ; Allocate stack space
100002f4: f000 fa1c    bl      0x10000730 <gpio_init>      ; Initialize LED GPIO
```

**LoRa Initialization:**
```assembly
10000614 <LoRa_init>:
10000614: b5f8         push    {r3, r4, r5, r6, r7, lr}   ; Save registers
10000616: 4647         mov     r7, r8
10000618: 46ce         mov     lr, r9
1000061a: 0004         movs    r4, r0                      ; Motor structure ptr
1000061c: b580         push    {r7, lr}
1000061e: 4690         mov     r8, r2                      ; GPIO pin 2
10000620: 000f         movs    r7, r1                      ; GPIO pin 1
10000622: 001e         movs    r6, r3                      ; GPIO pin 3
10000624: 2800         cmp     r0, #0x0                    ; Check null pointer
10000626: d040         beq     0x100006aa <LoRa_init+0x96>  ; Branch if null
10000628: 6083         str     r3, [r0, #0x8]             ; Store pin 3
1000062a: 9b08         ldr     r3, [sp, #0x20]            ; Load pin 4 from stack
1000062c: 2501         movs    r5, #0x1                   ; Set enabled flag
1000062e: 60c3         str     r3, [r0, #0xc]             ; Store pin 4
10000630: 9b09         ldr     r3, [sp, #0x24]            ; Load more parameters
10000632: 6001         str     r1, [r0]                   ; Store pin 1
10000634: 6103         str     r3, [r0, #0x10]            ; Store parameter
10000636: 2300         movs    r3, #0x0                   ; Clear position
10000638: 6042         str     r2, [r0, #0x4]             ; Store pin 2
1000063a: 6143         str     r3, [r0, #0x14]            ; Clear position counter
1000063c: 7605         strb    r5, [r0, #0x18]            ; Set enabled flag
```

#### GPIO Control Implementation

**GPIO Register Manipulation:**
```assembly
; GPIO base address: 0xd0000000
; Set GPIO pin high: Write to GPIO_OUT_SET (offset 0x24)
; Clear GPIO pin: Write to GPIO_OUT_CLR (offset 0x28)

1000065a: 002b         movs    r3, r5                      ; Copy pin mask
1000065c: 21d0         movs    r1, #0xd0                   ; GPIO base (high)
1000065e: 40bb         lsls    r3, r7                      ; Shift mask to pin
10000660: 0609         lsls    r1, r1, #0x18              ; Complete GPIO base
10000662: 624b         str     r3, [r1, #0x24]            ; Write to GPIO_OUT_SET
```

#### String Analysis

**Embedded Debug Strings:**
```c
// Found at addresses in .rodata section:
"Failed to initialize LoRa motor %d"
"LoRa motor initialization failed. Exiting..."
"LoRa motor %d initialized on pins %d,%d,%d,%d"
"All LoRa motors initialized successfully!"
"Starting LED blink and LoRa motor control loop..."
"Running LoRa motor demonstration sequence..."
```

#### Function Symbol Table

**Key Functions (272 total):**
```
Address   Type  Function Name
10000000  T     __boot2_start__
100001e8  T     _entry_point
100002d4  T     main
100002e4  T     run
10000414  t     LoRa_rotate_multiple_degrees.part.0
10000614  T     LoRa_init
100006b0  T     LoRa_demo_sequence
10000730  T     gpio_init
```

### Assembly Analysis Insights

#### ARM Cortex-M0+ Optimization Patterns

**1. Register Usage Optimization:**
- Frequent use of high registers (r8-r11) for temporary storage
- Stack manipulation for parameter passing
- Efficient register spilling during function calls

**2. GPIO Bit Manipulation:**
```assembly
; Efficient bit setting using shifts and masks
40bb         lsls    r3, r7          ; Create pin mask
624b         str     r3, [r1, #0x24] ; Atomic GPIO set
```

**3. Function Inlining:**
- Critical LoRa control functions partially inlined
- Reduced call overhead for time-sensitive operations

**4. Thumb-2 Instruction Usage:**
- 16-bit instructions for common operations
- 32-bit instructions for complex immediate values
- Optimal code density for Cortex-M0+

#### Performance Analysis

**Timing Characteristics:**
- GPIO switching: ~8 CPU cycles (60ns at 133MHz)
- Function call overhead: ~6 cycles
- Stack frame setup: ~4 cycles
- LoRa step sequence: ~400 cycles total

**Memory Efficiency:**
- Code size: 16.5KB (0.8% of flash)
- RAM usage: 3.5KB (1.3% of SRAM)
- No dynamic allocation in critical paths
- Efficient data structure packing

### Advanced Reverse Engineering Techniques

#### Control Flow Analysis

**Main Program Flow:**
```
main() → stdio_init_all() → run()
  ↓
LED GPIO initialization
  ↓
LoRa motor initialization (4 motors)
  ↓
Main control loop:
  ├── LED toggle every 500ms
  ├── Serial output status
  └── LoRa demo every 5 seconds
```

**LoRa Control Flow:**
```
LoRa_init() → gpio_init() (for each pin)
  ↓
LoRa_demo_sequence()
  ↓
LoRa_rotate_multiple_degrees()
  ↓
GPIO bit manipulation (4-phase sequence)
```

#### Security Analysis

**Attack Vectors:**
1. **GPIO Manipulation**: Direct hardware register access
2. **Timing Analysis**: Predictable step sequences
3. **Debug Interface**: UART communication exposure
4. **Memory Layout**: Predictable function addresses

**Defensive Measures:**
- Input validation on GPIO parameters
- Bounds checking for LoRa commands
- Disable debug output in production
- Use address randomization if available

#### Compiler Optimization Analysis

**GCC Optimization Flags Detected:**
- `-O2` optimization level (inferred from code patterns)
- Function inlining for performance-critical code
- Dead code elimination
- Constant folding for GPIO addresses

**Optimization Evidence:**
```assembly
; Immediate value optimization
21d0         movs    r1, #0xd0       ; Instead of loading from memory
0609         lsls    r1, r1, #0x18   ; Shifted to create 0xd0000000
```

## Troubleshooting

### Common Issues
- **Motors not moving**: Check 5V power connections to ULN2003 VCC
- **Weak rotation**: Ensure adequate power supply (USB 2.0+ recommended)
- **No serial output**: Check USB connection and terminal settings
- **Compilation errors**: Verify Pico SDK installation and environment

### Power Supply Notes
- USB 2.0 provides adequate current for 4 motors
- USB 1.1 or weak power supplies may cause erratic behavior
- External 5V supply can be used for higher current applications

## LoRa Remote Control

This project supports **two modes** using the same codebase:

### **Receiver Mode** (LoRa Controller - Default)
- Controls 4 LoRa motors via LoRa commands
- Listens for "ON" and "OFF" commands
- Provides acknowledgment responses

### **Transmitter Mode** (Remote Control)
- 2-button handheld remote (ON/OFF)
- Uses internal pull-ups (no external resistors needed)
- Sends LoRa commands to LoRa controller

### Build Instructions
```bash
# Build receiver mode (LoRa controller)
cd build
cmake -G "Unix Makefiles" ..
make
# Output: lora_receiver.uf2

# Build transmitter mode (remote control)
cd build
cmake -G "Unix Makefiles" -DBUILD_TRANSMITTER=ON ..
make
# Output: lora_transmitter.uf2
```

### Documentation
- **[LORA_WIRING.md](LORA_WIRING.md)**: LoRa module wiring and configuration
- **[LORA_REMOTE_CONTROL.md](LORA_REMOTE_CONTROL.md)**: Complete setup and usage guide
- **[LORA_WIRING.md](LoRa_WIRING.md)**: LoRa motor wiring diagrams

## License

Copyright (c) 2025 Kevin Thomas

## Contributing

This project follows professional embedded C standards with comprehensive documentation and modular design principles.
