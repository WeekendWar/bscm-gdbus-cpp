#pragma once

#include "Common.h"

class NotificationHandler {
private:
    GDBusConnection* connection_;
    std::string characteristic_path_;
    NotificationCallback callback_;
    guint properties_changed_subscription_;
    
    // D-Bus signal handler
    static void on_properties_changed(GDBusConnection* connection,
                                     const gchar* sender_name,
                                     const gchar* object_path,
                                     const gchar* interface_name,
                                     const gchar* signal_name,
                                     GVariant* parameters,
                                     gpointer user_data);
    
    // Handle the actual notification
    void handle_properties_changed(GVariant* changed_properties);

public:
    NotificationHandler(GDBusConnection* connection, const std::string& characteristic_path);
    ~NotificationHandler();
    
    // Enable/disable notifications
    bool enable_notifications(NotificationCallback callback);
    bool disable_notifications();
    
    // Properties
    const std::string& get_characteristic_path() const { return characteristic_path_; }
    bool is_enabled() const { return properties_changed_subscription_ != 0; }
};