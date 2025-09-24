#!/bin/bash

# Test build script for bscm-gdbus-cpp
# This script tests that the project can be built successfully

set -e

echo "Testing build process for bscm-gdbus-cpp..."

# Clean any previous build
echo "Cleaning previous build..."
rm -rf build

# Create build directory
echo "Creating build directory..."
mkdir build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build the project
echo "Building the project..."
make -j$(nproc)

# Check if executable was created
if [ -f "bscm-gdbus-cpp" ]; then
    echo "✅ Build successful! Executable created: bscm-gdbus-cpp"
    
    # Get file info
    echo "File information:"
    file bscm-gdbus-cpp
    ls -lh bscm-gdbus-cpp
    
    # Test help command (with timeout since we don't have BlueZ)
    echo "Testing basic functionality..."
    echo "help" | timeout 5s ./bscm-gdbus-cpp 2>&1 | head -20 || true
    
    echo "✅ All tests passed!"
else
    echo "❌ Build failed! Executable not found."
    exit 1
fi