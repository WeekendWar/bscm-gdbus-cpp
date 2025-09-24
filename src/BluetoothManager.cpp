#include "BluetoothManager.h"
#include <algorithm>

BluetoothManager::BluetoothManager() 
    : connection_(nullptr), is_scanning_(false) {
}

BluetoothManager::~BluetoothManager() {
    cleanup();
}

bool BluetoothManager::initialize() {
    GError* error = nullptr;
    
    // Connect to the system D-Bus
    connection_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (!connection_) {
        if (error) {
            std::cerr << "Failed to connect to D-Bus: " << error->message << std::endl;
            g_error_free(error);
        }
        return false;
    }
    
    // Find the default adapter
    adapter_path_ = find_default_adapter();
    if (adapter_path_.empty()) {
        std::cerr << "No Bluetooth adapter found" << std::endl;
        return false;
    }
    
    Utils::print_with_timestamp("Using adapter: " + adapter_path_);
    
    // Subscribe to D-Bus signals for device discovery
    g_dbus_connection_signal_subscribe(
        connection_,
        BlueZ::SERVICE_NAME,
        BlueZ::OBJECT_MANAGER_INTERFACE,
        "InterfacesAdded",
        nullptr,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_interfaces_added,
        this,
        nullptr
    );
    
    g_dbus_connection_signal_subscribe(
        connection_,
        BlueZ::SERVICE_NAME,
        BlueZ::OBJECT_MANAGER_INTERFACE,
        "InterfacesRemoved",
        nullptr,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_interfaces_removed,
        this,
        nullptr
    );
    
    g_dbus_connection_signal_subscribe(
        connection_,
        BlueZ::SERVICE_NAME,
        BlueZ::PROPERTIES_INTERFACE,
        "PropertiesChanged",
        nullptr,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_properties_changed,
        this,
        nullptr
    );
    
    // Ensure adapter is powered on
    return ensure_adapter_powered();
}

void BluetoothManager::cleanup() {
    if (connection_) {
        stop_discovery();
        devices_.clear();
        g_object_unref(connection_);
        connection_ = nullptr;
    }
}

std::string BluetoothManager::find_default_adapter() {
    if (!connection_) return "";
    
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        "/",
        BlueZ::OBJECT_MANAGER_INTERFACE,
        "GetManagedObjects",
        nullptr,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to get managed objects: " << error->message << std::endl;
            g_error_free(error);
        }
        return "";
    }
    
    GVariantIter* objects_iter;
    g_variant_get(result, "(a{oa{sa{sv}}})", &objects_iter);
    
    const gchar* object_path;
    GVariant* interfaces_dict;
    
    std::string adapter_path;
    while (g_variant_iter_loop(objects_iter, "{&o@a{sa{sv}}}", &object_path, &interfaces_dict)) {
        if (g_str_has_prefix(object_path, BlueZ::ADAPTER_PATH_PREFIX)) {
            GVariantIter* interfaces_iter;
            g_variant_get(interfaces_dict, "a{sa{sv}}", &interfaces_iter);
            
            const gchar* interface_name;
            GVariant* properties_dict;
            
            while (g_variant_iter_loop(interfaces_iter, "{&s@a{sv}}", &interface_name, &properties_dict)) {
                if (g_strcmp0(interface_name, BlueZ::ADAPTER_INTERFACE) == 0) {
                    adapter_path = object_path;
                    break;
                }
            }
            
            g_variant_iter_free(interfaces_iter);
            
            if (!adapter_path.empty()) {
                break;
            }
        }
    }
    
    g_variant_iter_free(objects_iter);
    g_variant_unref(result);
    
    return adapter_path;
}

bool BluetoothManager::ensure_adapter_powered() {
    if (adapter_path_.empty()) return false;
    
    // Check if already powered
    if (is_adapter_powered()) {
        return true;
    }
    
    return power_on_adapter();
}

bool BluetoothManager::power_on_adapter() {
    if (!connection_ || adapter_path_.empty()) return false;
    
    GError* error = nullptr;
    
    GVariant* powered_value = g_variant_new_boolean(TRUE);
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        adapter_path_.c_str(),
        BlueZ::PROPERTIES_INTERFACE,
        "Set",
        g_variant_new("(ssv)", BlueZ::ADAPTER_INTERFACE, "Powered", powered_value),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to power on adapter: " << error->message << std::endl;
            g_error_free(error);
        }
        return false;
    }
    
    g_variant_unref(result);
    
    // Wait a moment for the adapter to power on
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    return is_adapter_powered();
}

bool BluetoothManager::power_off_adapter() {
    if (!connection_ || adapter_path_.empty()) return false;
    
    // Stop discovery first
    stop_discovery();
    
    GError* error = nullptr;
    
    GVariant* powered_value = g_variant_new_boolean(FALSE);
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        adapter_path_.c_str(),
        BlueZ::PROPERTIES_INTERFACE,
        "Set",
        g_variant_new("(ssv)", BlueZ::ADAPTER_INTERFACE, "Powered", powered_value),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to power off adapter: " << error->message << std::endl;
            g_error_free(error);
        }
        return false;
    }
    
    g_variant_unref(result);
    return true;
}

bool BluetoothManager::is_adapter_powered() {
    if (!connection_ || adapter_path_.empty()) return false;
    
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        adapter_path_.c_str(),
        BlueZ::PROPERTIES_INTERFACE,
        "Get",
        g_variant_new("(ss)", BlueZ::ADAPTER_INTERFACE, "Powered"),
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
        return false;
    }
    
    GVariant* value;
    g_variant_get(result, "(v)", &value);
    
    gboolean powered = g_variant_get_boolean(value);
    
    g_variant_unref(value);
    g_variant_unref(result);
    
    return powered;
}

bool BluetoothManager::start_discovery(const std::vector<std::string>& service_uuids) {
    if (!connection_ || adapter_path_.empty()) return false;
    
    std::lock_guard<std::mutex> lock(scan_mutex_);
    
    if (is_scanning_) {
        stop_discovery();
    }
    
    target_service_uuids_ = service_uuids;
    
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        adapter_path_.c_str(),
        BlueZ::ADAPTER_INTERFACE,
        "StartDiscovery",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            std::cerr << "Failed to start discovery: " << error->message << std::endl;
            g_error_free(error);
        }
        return false;
    }
    
    g_variant_unref(result);
    is_scanning_ = true;
    
    return true;
}

bool BluetoothManager::stop_discovery() {
    if (!connection_ || adapter_path_.empty()) return false;
    
    std::lock_guard<std::mutex> lock(scan_mutex_);
    
    if (!is_scanning_) return true;
    
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        adapter_path_.c_str(),
        BlueZ::ADAPTER_INTERFACE,
        "StopDiscovery",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    
    if (!result) {
        if (error) {
            // Ignore errors - discovery might already be stopped
            g_error_free(error);
        }
    } else {
        g_variant_unref(result);
    }
    
    is_scanning_ = false;
    return true;
}

bool BluetoothManager::is_discovering() {
    std::lock_guard<std::mutex> lock(scan_mutex_);
    return is_scanning_;
}

void BluetoothManager::on_interfaces_added(GDBusConnection* connection,
                                         const gchar* sender_name,
                                         const gchar* object_path,
                                         const gchar* interface_name,
                                         const gchar* signal_name,
                                         GVariant* parameters,
                                         gpointer user_data) {
    (void)connection;
    (void)sender_name;
    (void)interface_name;
    (void)signal_name;
    
    BluetoothManager* manager = static_cast<BluetoothManager*>(user_data);
    
    GVariant* interfaces;
    g_variant_get(parameters, "(&o@a{sa{sv}})", &object_path, &interfaces);
    
    manager->handle_interfaces_added(object_path, interfaces);
    
    g_variant_unref(interfaces);
}

void BluetoothManager::on_interfaces_removed(GDBusConnection* connection,
                                           const gchar* sender_name,
                                           const gchar* object_path,
                                           const gchar* interface_name,
                                           const gchar* signal_name,
                                           GVariant* parameters,
                                           gpointer user_data) {
    (void)connection;
    (void)sender_name;
    (void)interface_name;
    (void)signal_name;
    
    BluetoothManager* manager = static_cast<BluetoothManager*>(user_data);
    
    GVariant* interfaces_array;
    g_variant_get(parameters, "(&o@as)", &object_path, &interfaces_array);
    
    GVariantIter iter;
    g_variant_iter_init(&iter, interfaces_array);
    const gchar* interface;
    std::vector<std::string> interfaces;
    
    while (g_variant_iter_loop(&iter, "&s", &interface)) {
        interfaces.push_back(interface);
    }
    
    manager->handle_interfaces_removed(object_path, interfaces);
    
    g_variant_unref(interfaces_array);
}

void BluetoothManager::on_properties_changed(GDBusConnection* connection,
                                            const gchar* sender_name,
                                            const gchar* object_path,
                                            const gchar* interface_name,
                                            const gchar* signal_name,
                                            GVariant* parameters,
                                            gpointer user_data) {
    (void)connection;
    (void)sender_name;
    (void)signal_name;
    
    BluetoothManager* manager = static_cast<BluetoothManager*>(user_data);
    
    const gchar* changed_interface;
    GVariant* changed_properties;
    GVariant* invalidated_properties;
    
    g_variant_get(parameters, "(&s@a{sv}@as)", &changed_interface, &changed_properties, &invalidated_properties);
    
    manager->handle_properties_changed(object_path, changed_interface, changed_properties);
    
    g_variant_unref(changed_properties);
    g_variant_unref(invalidated_properties);
}

void BluetoothManager::handle_interfaces_added(const std::string& object_path, GVariant* interfaces) {
    GVariantIter iter;
    g_variant_iter_init(&iter, interfaces);
    
    const gchar* interface_name;
    GVariant* properties;
    
    bool is_device = false;
    
    while (g_variant_iter_loop(&iter, "{&s@a{sv}}", &interface_name, &properties)) {
        if (g_strcmp0(interface_name, BlueZ::DEVICE_INTERFACE) == 0) {
            is_device = true;
            break;
        }
    }
    
    if (is_device) {
        // Check if device matches target service UUIDs (if specified)
        if (!target_service_uuids_.empty() && !device_has_target_service(object_path)) {
            return;
        }
        
        auto device = std::make_shared<BluetoothDevice>(connection_, object_path);
        
        // Extract address from device for indexing
        std::string address = device->get_address();
        if (!address.empty()) {
            devices_[address] = device;
            Utils::print_with_timestamp("Device discovered: " + device->get_name() + " (" + address + ")");
        }
    }
}

void BluetoothManager::handle_interfaces_removed(const std::string& object_path, const std::vector<std::string>& interfaces) {
    (void)interfaces;
    
    // Find device by path and remove it
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (it->second->get_object_path() == object_path) {
            Utils::print_with_timestamp("Device removed: " + it->second->get_name() + " (" + it->first + ")");
            devices_.erase(it);
            break;
        }
    }
}

void BluetoothManager::handle_properties_changed(const std::string& object_path, const std::string& interface_name, GVariant* changed_properties) {
    // Find the device and notify it of property changes
    for (auto& pair : devices_) {
        if (pair.second->get_object_path() == object_path) {
            if (interface_name == BlueZ::DEVICE_INTERFACE) {
                // Check for connection state changes
                GVariantIter iter;
                g_variant_iter_init(&iter, changed_properties);
                const gchar* key;
                GVariant* value;
                
                while (g_variant_iter_loop(&iter, "{&sv}", &key, &value)) {
                    if (g_strcmp0(key, "Connected") == 0) {
                        pair.second->update_connection_state(g_variant_get_boolean(value));
                    } else if (g_strcmp0(key, "ServicesResolved") == 0) {
                        pair.second->update_services_resolved_state(g_variant_get_boolean(value));
                    }
                }
            }
            break;
        }
    }
}

bool BluetoothManager::device_has_target_service(const std::string& device_path) {
    if (target_service_uuids_.empty()) return true;
    
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection_,
        BlueZ::SERVICE_NAME,
        device_path.c_str(),
        BlueZ::PROPERTIES_INTERFACE,
        "Get",
        g_variant_new("(ss)", BlueZ::DEVICE_INTERFACE, "UUIDs"),
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
        return false;
    }
    
    GVariant* value;
    g_variant_get(result, "(v)", &value);
    
    if (!g_variant_is_of_type(value, G_VARIANT_TYPE("as"))) {
        g_variant_unref(value);
        g_variant_unref(result);
        return false;
    }
    
    GVariantIter iter;
    g_variant_iter_init(&iter, value);
    const gchar* uuid;
    
    bool has_target_service = false;
    while (g_variant_iter_loop(&iter, "&s", &uuid)) {
        for (const auto& target_uuid : target_service_uuids_) {
            if (g_ascii_strcasecmp(uuid, target_uuid.c_str()) == 0) {
                has_target_service = true;
                break;
            }
        }
        if (has_target_service) break;
    }
    
    g_variant_unref(value);
    g_variant_unref(result);
    
    return has_target_service;
}

std::vector<std::shared_ptr<BluetoothDevice>> BluetoothManager::get_discovered_devices() {
    std::vector<std::shared_ptr<BluetoothDevice>> device_list;
    
    for (const auto& pair : devices_) {
        device_list.push_back(pair.second);
    }
    
    return device_list;
}

std::shared_ptr<BluetoothDevice> BluetoothManager::get_device(const std::string& address) {
    auto it = devices_.find(address);
    if (it != devices_.end()) {
        return it->second;
    }
    return nullptr;
}

bool BluetoothManager::remove_device(const std::string& address) {
    auto it = devices_.find(address);
    if (it != devices_.end()) {
        // Disconnect if connected
        if (it->second->is_connected()) {
            it->second->disconnect();
        }
        
        devices_.erase(it);
        return true;
    }
    return false;
}

void BluetoothManager::print_discovered_devices() {
    if (devices_.empty()) {
        Utils::print_with_timestamp("No devices discovered");
        return;
    }
    
    Utils::print_with_timestamp("Discovered devices:");
    for (const auto& pair : devices_) {
        const auto& device = pair.second;
        std::cout << "  " << device->get_address() << " - " << device->get_name()
                  << " (Connected: " << (device->is_connected() ? "Yes" : "No") << ")" << std::endl;
    }
}

void BluetoothManager::set_target_service_uuids(const std::vector<std::string>& uuids) {
    target_service_uuids_ = uuids;
}