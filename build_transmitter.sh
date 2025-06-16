#!/bin/bash

# Build LoRa Transmitter (Remote Control) Firmware
# This script builds the transmitter mode firmware for the remote control Pico

echo "=========================================="
echo "Building LoRa Transmitter (Remote Control)"
echo "=========================================="

# Clean previous build
echo "Cleaning previous build..."
rm -rf build
mkdir build
cd build

# Configure for transmitter mode
echo "Configuring CMake for transmitter mode..."
cmake -G "Unix Makefiles" -DBUILD_TRANSMITTER=ON ..

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
    echo "1. Put your TRANSMITTER Pico in bootloader mode (hold BOOTSEL + power)"
    echo "2. Copy LoRa.uf2 to the Pico drive"
    echo "3. Connect via USB and open serial monitor at 115200 baud"
    echo ""
    echo "This firmware is for the REMOTE CONTROL (2 buttons) Pico"
    echo "=========================================="
else
    echo "❌ Build failed!"
    exit 1
fi
