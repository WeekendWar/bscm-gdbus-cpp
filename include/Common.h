#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <glib.h>
#include <gio/gio.h>

// BlueZ D-Bus constants
namespace BlueZ {
    constexpr const char* SERVICE_NAME = "org.bluez";
    constexpr const char* ADAPTER_INTERFACE = "org.bluez.Adapter1";
    constexpr const char* DEVICE_INTERFACE = "org.bluez.Device1";
    constexpr const char* GATT_SERVICE_INTERFACE = "org.bluez.GattService1";
    constexpr const char* GATT_CHARACTERISTIC_INTERFACE = "org.bluez.GattCharacteristic1";
    constexpr const char* PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";
    constexpr const char* OBJECT_MANAGER_INTERFACE = "org.freedesktop.DBus.ObjectManager";
    
    constexpr const char* ADAPTER_PATH_PREFIX = "/org/bluez/hci";
    constexpr const char* DEVICE_PATH_PREFIX = "/org/bluez/hci";
}

// Common types
using NotificationCallback = std::function<void(const std::string& characteristic_path, const std::vector<uint8_t>& data)>;
using ErrorCallback = std::function<void(const std::string& error_message)>;

// Utility functions
namespace Utils {
    std::string bytes_to_hex_string(const std::vector<uint8_t>& data);
    std::vector<uint8_t> hex_string_to_bytes(const std::string& hex_str);
    std::string variant_to_string(GVariant* variant);
    std::vector<uint8_t> variant_to_bytes(GVariant* variant);
    void print_with_timestamp(const std::string& message);
}

// Exception class for Bluetooth operations
class BluetoothException : public std::exception {
private:
    std::string message_;

public:
    explicit BluetoothException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
};