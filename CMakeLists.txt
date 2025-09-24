cmake_minimum_required(VERSION 3.16)
project(bscm-gdbus-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

# Add include directories
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${GIO_INCLUDE_DIRS})

# Add compile definitions and flags
add_definitions(${GLIB_CFLAGS_OTHER})
add_definitions(${GIO_CFLAGS_OTHER})
add_compile_options(-Wall -Wextra -pedantic)

# Create source directory structure
set(SRC_DIR src)
set(INCLUDE_DIR include)

# Include directories
include_directories(${INCLUDE_DIR})

# Source files
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/Common.cpp
    ${SRC_DIR}/BluetoothManager.cpp
    ${SRC_DIR}/BluetoothDevice.cpp
    ${SRC_DIR}/GattCharacteristic.cpp
    ${SRC_DIR}/NotificationHandler.cpp
)

# Header files
set(HEADERS
    ${INCLUDE_DIR}/BluetoothManager.h
    ${INCLUDE_DIR}/BluetoothDevice.h
    ${INCLUDE_DIR}/GattCharacteristic.h
    ${INCLUDE_DIR}/NotificationHandler.h
    ${INCLUDE_DIR}/Common.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    ${GLIB_LIBRARIES} 
    ${GIO_LIBRARIES}
)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)