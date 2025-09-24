#include "NotificationHandler.h"

NotificationHandler::NotificationHandler(GDBusConnection* connection, const std::string& characteristic_path)
    : connection_(connection), characteristic_path_(characteristic_path), properties_changed_subscription_(0) {
    if (connection_) {
        g_object_ref(connection_);
    }
}

NotificationHandler::~NotificationHandler() {
    disable_notifications();
    
    if (connection_) {
        g_object_unref(connection_);
    }
}

bool NotificationHandler::enable_notifications(NotificationCallback callback) {
    if (!connection_ || properties_changed_subscription_ != 0) {
        return false;
    }
    
    callback_ = callback;
    
    // Subscribe to PropertiesChanged signals for this characteristic
    properties_changed_subscription_ = g_dbus_connection_signal_subscribe(
        connection_,
        BlueZ::SERVICE_NAME,
        BlueZ::PROPERTIES_INTERFACE,
        "PropertiesChanged",
        characteristic_path_.c_str(),
        BlueZ::GATT_CHARACTERISTIC_INTERFACE,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_properties_changed,
        this,
        nullptr
    );
    
    return properties_changed_subscription_ != 0;
}

bool NotificationHandler::disable_notifications() {
    if (!connection_ || properties_changed_subscription_ == 0) {
        return true;
    }
    
    g_dbus_connection_signal_unsubscribe(connection_, properties_changed_subscription_);
    properties_changed_subscription_ = 0;
    callback_ = nullptr;
    
    return true;
}

void NotificationHandler::on_properties_changed(GDBusConnection* connection,
                                               const gchar* sender_name,
                                               const gchar* object_path,
                                               const gchar* interface_name,
                                               const gchar* signal_name,
                                               GVariant* parameters,
                                               gpointer user_data) {
    (void)connection;
    (void)sender_name;
    (void)object_path;
    (void)interface_name;
    (void)signal_name;
    
    NotificationHandler* handler = static_cast<NotificationHandler*>(user_data);
    
    const gchar* changed_interface;
    GVariant* changed_properties;
    GVariant* invalidated_properties;
    
    g_variant_get(parameters, "(&s@a{sv}@as)", &changed_interface, &changed_properties, &invalidated_properties);
    
    // Only handle GATT characteristic interface changes
    if (g_strcmp0(changed_interface, BlueZ::GATT_CHARACTERISTIC_INTERFACE) == 0) {
        handler->handle_properties_changed(changed_properties);
    }
    
    g_variant_unref(changed_properties);
    g_variant_unref(invalidated_properties);
}

void NotificationHandler::handle_properties_changed(GVariant* changed_properties) {
    GVariantIter iter;
    g_variant_iter_init(&iter, changed_properties);
    
    const gchar* key;
    GVariant* value;
    
    while (g_variant_iter_loop(&iter, "{&sv}", &key, &value)) {
        if (g_strcmp0(key, "Value") == 0) {
            // This is a notification with new data
            std::vector<uint8_t> data = Utils::variant_to_bytes(value);
            
            if (callback_) {
                callback_(characteristic_path_, data);
            }
        }
    }
}