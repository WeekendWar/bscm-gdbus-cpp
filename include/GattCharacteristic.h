#pragma once

#include "Common.h"
#include "NotificationHandler.h"

class GattCharacteristic
{
private:
  GDBusConnection*                     connection_;
  std::string                          object_path_;
  std::string                          service_path_;
  std::string                          uuid_;
  std::vector<std::string>             flags_;
  std::shared_ptr<NotificationHandler> notification_handler_;
  bool                                 notifications_enabled_;

  // D-Bus callbacks
  static void on_read_ready(GObject*      source_object,
                            GAsyncResult* result,
                            gpointer      user_data);
  static void on_write_ready(GObject*      source_object,
                             GAsyncResult* result,
                             gpointer      user_data);
  static void on_start_notify_ready(GObject*      source_object,
                                    GAsyncResult* result,
                                    gpointer      user_data);
  static void on_stop_notify_ready(GObject*      source_object,
                                   GAsyncResult* result,
                                   gpointer      user_data);

  // Helper methods
  GVariant* get_property(const std::string& property);
  bool      set_property(const std::string& property, GVariant* value);
  void      update_properties();

public:
  GattCharacteristic(GDBusConnection*   connection,
                     const std::string& object_path);
  ~GattCharacteristic();

  // Basic properties
  const std::string& get_uuid() const { return uuid_; }
  const std::string& get_object_path() const { return object_path_; }
  const std::string& get_service_path() const { return service_path_; }
  const std::vector<std::string>& get_flags() const { return flags_; }

  // GATT operations
  bool read_value(std::vector<uint8_t>& data);
  bool write_value(const std::vector<uint8_t>& data);
  bool start_notifications(NotificationCallback callback);
  bool stop_notifications();

  // Properties
  bool can_read() const;
  bool can_write() const;
  bool can_write_without_response() const;
  bool can_notify() const;
  bool can_indicate() const;
  bool are_notifications_enabled() const { return notifications_enabled_; }

  // Utility
  void        print_characteristic_info();
  std::string flags_to_string() const;

  // Called by NotificationHandler when notification is received
  void handle_notification(const std::vector<uint8_t>& data);
};