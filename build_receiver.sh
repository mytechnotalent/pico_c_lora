#!/bin/bash

# Build LoRa Receiver (LoRa Controller) Firmware
# This script builds the receiver mode firmware for the LoRa motor controller Pico

echo "========================================"
echo "Building LoRa Receiver (LoRa Controller)"
echo "========================================"

# Clean previous build
echo "Cleaning previous build..."
rm -rf build
mkdir build
cd build

# Configure for receiver mode (default)
echo "Configuring CMake for receiver mode..."
cmake -G "Unix Makefiles" ..

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed!"
    exit 1
fi

# Build the project
echo "Building firmware..."
make

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo "📁 Output file: build/LoRa.uf2"
    echo ""
    echo "🔧 Next steps:"
    echo "1. Put your RECEIVER Pico in bootloader mode (hold BOOTSEL + power)"
    echo "2. Copy LoRa.uf2 to the Pico drive"
    echo "3. Connect LoRa motors and LoRa module"
    echo "4. Power on and check serial output at 115200 baud"
    echo ""
    echo "This firmware is for the LoRa CONTROLLER (4 motors + LoRa) Pico"
    echo "========================================"
else
    echo "❌ Build failed!"
    exit 1
fi
