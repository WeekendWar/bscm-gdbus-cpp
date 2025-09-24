#pragma once

#include "Common.h"
#include "GattCharacteristic.h"

class BluetoothDevice
{
private:
  GDBusConnection*                                           connection_;
  std::string                                                object_path_;
  std::string                                                address_;
  std::string                                                name_;
  bool                                                       connected_;
  bool                                                       services_resolved_;
  std::vector<std::string>                                   service_uuids_;
  std::map<std::string, std::shared_ptr<GattCharacteristic>> characteristics_;

  // D-Bus callback for async operations
  static void on_device_connect_ready(GObject*      source_object,
                                      GAsyncResult* result,
                                      gpointer      user_data);
  static void on_device_disconnect_ready(GObject*      source_object,
                                         GAsyncResult* result,
                                         gpointer      user_data);

  // Helper methods
  void      update_properties();
  void      discover_services_and_characteristics();
  GVariant* get_property(const std::string& interface,
                         const std::string& property);
  bool      set_property(const std::string& interface,
                         const std::string& property,
                         GVariant*          value);

public:
  BluetoothDevice(GDBusConnection* connection, const std::string& object_path);
  ~BluetoothDevice();

  // Basic properties
  const std::string& get_address() const { return address_; }
  const std::string& get_name() const { return name_; }
  const std::string& get_object_path() const { return object_path_; }
  bool               is_connected() const { return connected_; }
  bool are_services_resolved() const { return services_resolved_; }
  const std::vector<std::string>& get_service_uuids() const
  {
    return service_uuids_;
  }

  // Connection management
  bool connect();
  bool disconnect();
  bool pair();
  bool unpair();

  // Service and characteristic discovery
  bool                                             refresh_services();
  std::vector<std::shared_ptr<GattCharacteristic>> get_characteristics();
  std::shared_ptr<GattCharacteristic>              get_characteristic(
                 const std::string& service_uuid,
                 const std::string& char_uuid);
  std::shared_ptr<GattCharacteristic> get_characteristic_by_path(
    const std::string& char_path);

  // GATT operations
  bool read_characteristic(const std::string&    service_uuid,
                           const std::string&    char_uuid,
                           std::vector<uint8_t>& data);
  bool write_characteristic(const std::string&          service_uuid,
                            const std::string&          char_uuid,
                            const std::vector<uint8_t>& data);
  bool subscribe_to_notifications(const std::string&   service_uuid,
                                  const std::string&   char_uuid,
                                  NotificationCallback callback);
  bool unsubscribe_from_notifications(const std::string& service_uuid,
                                      const std::string& char_uuid);

  // Utility
  void print_device_info();
  void print_services_and_characteristics();
  bool has_service(const std::string& service_uuid);

  // Update device state from D-Bus signals
  void update_connection_state(bool connected);
  void update_services_resolved_state(bool resolved);
};