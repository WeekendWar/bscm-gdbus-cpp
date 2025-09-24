#include "GattCharacteristic.h"
#include <algorithm>

GattCharacteristic::GattCharacteristic(GDBusConnection* connection, const std::string& object_path)
    : connection_(connection), object_path_(object_path), notifications_enabled_(false) {
    if (connection_) {
        g_object_ref(connection_);
    }
    update_properties();
}

GattCharacteristic::~GattCharacteristic() {
    if (notifications_enabled_) {
        stop_notifications();
    }
    
    if (connection_) {
        g_object_unref(connection_);
    }
}

void GattCharacteristic::update_properties() {
    // Get UUID
    auto uuid_var = get_property("UUID");
    if (uuid_var) {
        uuid_ = g_variant_get_string(uuid_var, nullptr);
        g_variant_unref(uuid_var);
    }
    
    // Get Service path
    auto service_var = get_property("Service");
    if (service_var) {
        service_path_ = g_variant_get_string(service_var, nullptr);
        g_variant_unref(service_var);
    }
    
    // Get Flags
    auto flags_var = get_property("Flags");
    if (flags_var) {
        flags_.clear();
        
        GVariantIter iter;
        g_variant_iter_init(&iter, flags_var);
        const gchar* flag;
        
        while (g_variant_iter_loop(&iter, "&s", &flag)) {
            flags_.push_back(flag);
        }
        
        g_variant_unref(flags_var);
    }
}

GVariant* GattCharacteristic::get_property(const std::string& property) {
    if (!connection_) return nullptr;
    
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        object_path_.c_str(),
        BlueZ::PROPERTIES_INTERFACE,
        "Get",
        g_variant_new("(ss)", BlueZ::GATT_CHARACTERISTIC_INTERFACE, property.c_str()),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            g_error_free(error);
        }
        return nullptr;
    }
    
    GVariant* value;
    g_variant_get(result, "(v)", &value);
    g_variant_unref(result);
    
    return value;
}

bool GattCharacteristic::set_property(const std::string& property, GVariant* value) {
    if (!connection_) return false;
    
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        object_path_.c_str(),
        BlueZ::PROPERTIES_INTERFACE,
        "Set",
        g_variant_new("(ssv)", BlueZ::GATT_CHARACTERISTIC_INTERFACE, property.c_str(), value),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to set characteristic property " << property << ": " << error->message << std::endl;
            g_error_free(error);
        }
        return false;
    }
    
    g_variant_unref(result);
    return true;
}

bool GattCharacteristic::read_value(std::vector<uint8_t>& data) {
    if (!connection_ || !can_read()) {
        Utils::print_with_timestamp("Characteristic does not support reading");
        return false;
    }
    
    GError* error = nullptr;
    
    // Create empty options dict
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    GVariant* options = g_variant_builder_end(&builder);
    
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        object_path_.c_str(),
        BlueZ::GATT_CHARACTERISTIC_INTERFACE,
        "ReadValue",
        g_variant_new_tuple(&options, 1),
        G_VARIANT_TYPE("(ay)"),
        G_DBUS_CALL_FLAGS_NONE,
        10000,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to read characteristic: " << error->message << std::endl;
            g_error_free(error);
        }
        return false;
    }
    
    GVariant* value_array;
    g_variant_get(result, "(@ay)", &value_array);
    
    data = Utils::variant_to_bytes(value_array);
    
    g_variant_unref(value_array);
    g_variant_unref(result);
    
    return true;
}

bool GattCharacteristic::write_value(const std::vector<uint8_t>& data) {
    if (!connection_ || (!can_write() && !can_write_without_response())) {
        Utils::print_with_timestamp("Characteristic does not support writing");
        return false;
    }
    
    GError* error = nullptr;
    
    // Create byte array from data
    GVariant* data_variant = g_variant_new_fixed_array(
        G_VARIANT_TYPE_BYTE,
        data.data(),
        data.size(),
        sizeof(uint8_t)
    );
    
    // Create empty options dict
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    GVariant* options = g_variant_builder_end(&builder);
    
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        object_path_.c_str(),
        BlueZ::GATT_CHARACTERISTIC_INTERFACE,
        "WriteValue",
        g_variant_new("(@aya{sv})", data_variant, options),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        10000,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to write characteristic: " << error->message << std::endl;
            g_error_free(error);
        }
        return false;
    }
    
    g_variant_unref(result);
    return true;
}

bool GattCharacteristic::start_notifications(NotificationCallback callback) {
    if (!connection_ || !can_notify()) {
        Utils::print_with_timestamp("Characteristic does not support notifications");
        return false;
    }
    
    if (notifications_enabled_) {
        stop_notifications();
    }
    
    // Create notification handler
    notification_handler_ = std::make_shared<NotificationHandler>(connection_, object_path_);
    
    if (!notification_handler_->enable_notifications([this, callback](const std::string& char_path, const std::vector<uint8_t>& data) {
        if (callback) {
            callback(char_path, data);
        }
    })) {
        notification_handler_.reset();
        return false;
    }
    
    // Call StartNotify on the characteristic
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        object_path_.c_str(),
        BlueZ::GATT_CHARACTERISTIC_INTERFACE,
        "StartNotify",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        10000,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to start notifications: " << error->message << std::endl;
            g_error_free(error);
        }
        notification_handler_.reset();
        return false;
    }
    
    g_variant_unref(result);
    notifications_enabled_ = true;
    
    return true;
}

bool GattCharacteristic::stop_notifications() {
    if (!connection_ || !notifications_enabled_) return true;
    
    // Call StopNotify on the characteristic
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        object_path_.c_str(),
        BlueZ::GATT_CHARACTERISTIC_INTERFACE,
        "StopNotify",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        10000,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to stop notifications: " << error->message << std::endl;
            g_error_free(error);
        }
        // Continue with cleanup even if the call failed
    } else {
        g_variant_unref(result);
    }
    
    // Disable notification handler
    if (notification_handler_) {
        notification_handler_->disable_notifications();
        notification_handler_.reset();
    }
    
    notifications_enabled_ = false;
    return true;
}

bool GattCharacteristic::can_read() const {
    return std::find(flags_.begin(), flags_.end(), "read") != flags_.end();
}

bool GattCharacteristic::can_write() const {
    return std::find(flags_.begin(), flags_.end(), "write") != flags_.end();
}

bool GattCharacteristic::can_write_without_response() const {
    return std::find(flags_.begin(), flags_.end(), "write-without-response") != flags_.end();
}

bool GattCharacteristic::can_notify() const {
    return std::find(flags_.begin(), flags_.end(), "notify") != flags_.end();
}

bool GattCharacteristic::can_indicate() const {
    return std::find(flags_.begin(), flags_.end(), "indicate") != flags_.end();
}

void GattCharacteristic::print_characteristic_info() {
    std::cout << "\n=== Characteristic Information ===" << std::endl;
    std::cout << "UUID: " << uuid_ << std::endl;
    std::cout << "Object Path: " << object_path_ << std::endl;
    std::cout << "Service Path: " << service_path_ << std::endl;
    std::cout << "Flags: " << flags_to_string() << std::endl;
    std::cout << "Notifications Enabled: " << (notifications_enabled_ ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

std::string GattCharacteristic::flags_to_string() const {
    if (flags_.empty()) {
        return "None";
    }
    
    std::string result;
    for (size_t i = 0; i < flags_.size(); ++i) {
        result += flags_[i];
        if (i < flags_.size() - 1) {
            result += ", ";
        }
    }
    
    return result;
}

void GattCharacteristic::handle_notification(const std::vector<uint8_t>& data) {
    Utils::print_with_timestamp("Notification received for " + uuid_ + ": " + 
                               Utils::bytes_to_hex_string(data));
}