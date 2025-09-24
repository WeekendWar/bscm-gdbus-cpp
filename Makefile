# Simple Makefile for convenience
# Use CMake for actual building

.PHONY: all build clean install run help

# Default target
all: build

# Build the project
build:
	@mkdir -p build
	@cd build && cmake .. && make -j$$(nproc)

# Clean build files
clean:
	@rm -rf build

# Install the application
install: build
	@cd build && make install

# Run the application
run: build
	@./build/bscm-gdbus-cpp

# Show help
help:
	@echo "Available targets:"
	@echo "  all     - Build the project (default)"
	@echo "  build   - Build the project"
	@echo "  clean   - Clean build files"
	@echo "  install - Install the application"
	@echo "  run     - Build and run the application"
	@echo "  help    - Show this help message"

# Debugging target
debug: 
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$$(nproc)

# Release target  
release:
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$$(nproc)