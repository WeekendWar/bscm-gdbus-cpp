#pragma once

#include "BluetoothDevice.h"
#include "Common.h"

class BluetoothManager
{
private:
  GDBusConnection*                                        connection_;
  std::string                                             adapter_path_;
  bool                                                    is_scanning_;
  std::mutex                                              scan_mutex_;
  std::map<std::string, std::shared_ptr<BluetoothDevice>> devices_;
  std::vector<std::string>                                target_service_uuids_;

  // D-Bus signal handlers
  static void on_interfaces_added(GDBusConnection* connection,
                                  const gchar*     sender_name,
                                  const gchar*     object_path,
                                  const gchar*     interface_name,
                                  const gchar*     signal_name,
                                  GVariant*        parameters,
                                  gpointer         user_data);

  static void on_interfaces_removed(GDBusConnection* connection,
                                    const gchar*     sender_name,
                                    const gchar*     object_path,
                                    const gchar*     interface_name,
                                    const gchar*     signal_name,
                                    GVariant*        parameters,
                                    gpointer         user_data);

  static void on_properties_changed(GDBusConnection* connection,
                                    const gchar*     sender_name,
                                    const gchar*     object_path,
                                    const gchar*     interface_name,
                                    const gchar*     signal_name,
                                    GVariant*        parameters,
                                    gpointer         user_data);

  // Helper methods
  void handle_interfaces_added(const std::string& object_path,
                               GVariant*          interfaces);
  void handle_interfaces_removed(const std::string&              object_path,
                                 const std::vector<std::string>& interfaces);
  void handle_properties_changed(const std::string& object_path,
                                 const std::string& interface_name,
                                 GVariant*          changed_properties);

  bool        device_has_target_service(const std::string& device_path);
  std::string find_default_adapter();
  bool        ensure_adapter_powered();

public:
  BluetoothManager();
  ~BluetoothManager();

  // Initialize connection to BlueZ
  bool initialize();
  void cleanup();

  // Adapter management
  bool power_on_adapter();
  bool power_off_adapter();
  bool is_adapter_powered();

  // Scanning operations
  bool start_discovery(const std::vector<std::string>& service_uuids = {});
  bool stop_discovery();
  bool is_discovering();

  // Device management
  std::vector<std::shared_ptr<BluetoothDevice>> get_discovered_devices();
  std::shared_ptr<BluetoothDevice> get_device(const std::string& address);
  bool                             remove_device(const std::string& address);

  // Utility
  void print_discovered_devices();
  void set_target_service_uuids(const std::vector<std::string>& uuids);

  // Get the D-Bus connection for devices to use
  GDBusConnection* get_connection() const { return connection_; }
};