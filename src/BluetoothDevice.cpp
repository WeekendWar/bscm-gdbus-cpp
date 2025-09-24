#include "BluetoothDevice.h"

BluetoothDevice::BluetoothDevice(GDBusConnection*   connection,
                                 const std::string& object_path)
  : connection_(connection)
  , object_path_(object_path)
  , connected_(false)
  , services_resolved_(false)
{
  if (connection_)
  {
    g_object_ref(connection_);
  }
  update_properties();
}

BluetoothDevice::~BluetoothDevice()
{
  if (connected_)
  {
    disconnect();
  }

  if (connection_)
  {
    g_object_unref(connection_);
  }
}

void BluetoothDevice::update_properties()
{
  // Get basic device properties
  auto address_var = get_property(BlueZ::DEVICE_INTERFACE, "Address");
  if (address_var)
  {
    address_ = g_variant_get_string(address_var, nullptr);
    g_variant_unref(address_var);
  }

  auto name_var = get_property(BlueZ::DEVICE_INTERFACE, "Name");
  if (name_var)
  {
    name_ = g_variant_get_string(name_var, nullptr);
    g_variant_unref(name_var);
  }
  else
  {
    // Fallback to alias if name is not available
    auto alias_var = get_property(BlueZ::DEVICE_INTERFACE, "Alias");
    if (alias_var)
    {
      name_ = g_variant_get_string(alias_var, nullptr);
      g_variant_unref(alias_var);
    }
    else
    {
      name_ = "Unknown Device";
    }
  }

  auto connected_var = get_property(BlueZ::DEVICE_INTERFACE, "Connected");
  if (connected_var)
  {
    connected_ = g_variant_get_boolean(connected_var);
    g_variant_unref(connected_var);
  }

  auto services_resolved_var =
    get_property(BlueZ::DEVICE_INTERFACE, "ServicesResolved");
  if (services_resolved_var)
  {
    services_resolved_ = g_variant_get_boolean(services_resolved_var);
    g_variant_unref(services_resolved_var);
  }

  // Get service UUIDs
  auto uuids_var = get_property(BlueZ::DEVICE_INTERFACE, "UUIDs");
  if (uuids_var)
  {
    service_uuids_.clear();

    GVariantIter iter;
    g_variant_iter_init(&iter, uuids_var);
    const gchar* uuid;

    while (g_variant_iter_loop(&iter, "&s", &uuid))
    {
      service_uuids_.push_back(uuid);
    }

    g_variant_unref(uuids_var);
  }
}

GVariant* BluetoothDevice::get_property(const std::string& interface,
                                        const std::string& property)
{
  if (!connection_)
    return nullptr;

  GError*   error  = nullptr;
  GVariant* result = g_dbus_connection_call_sync(
    connection_,
    BlueZ::SERVICE_NAME,
    object_path_.c_str(),
    BlueZ::PROPERTIES_INTERFACE,
    "Get",
    g_variant_new("(ss)", interface.c_str(), property.c_str()),
    G_VARIANT_TYPE("(v)"),
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    nullptr,
    &error);

  if (!result)
  {
    if (error)
    {
      g_error_free(error);
    }
    return nullptr;
  }

  GVariant* value;
  g_variant_get(result, "(v)", &value);
  g_variant_unref(result);

  return value;
}

bool BluetoothDevice::set_property(const std::string& interface,
                                   const std::string& property,
                                   GVariant*          value)
{
  if (!connection_)
    return false;

  GError*   error  = nullptr;
  GVariant* result = g_dbus_connection_call_sync(
    connection_,
    BlueZ::SERVICE_NAME,
    object_path_.c_str(),
    BlueZ::PROPERTIES_INTERFACE,
    "Set",
    g_variant_new("(ssv)", interface.c_str(), property.c_str(), value),
    nullptr,
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    nullptr,
    &error);

  if (!result)
  {
    if (error)
    {
      std::cerr << "Failed to set property " << property << ": "
                << error->message << std::endl;
      g_error_free(error);
    }
    return false;
  }

  g_variant_unref(result);
  return true;
}

bool BluetoothDevice::connect()
{
  if (!connection_ || connected_)
    return connected_;

  GError*   error  = nullptr;
  GVariant* result = g_dbus_connection_call_sync(connection_,
                                                 BlueZ::SERVICE_NAME,
                                                 object_path_.c_str(),
                                                 BlueZ::DEVICE_INTERFACE,
                                                 "Connect",
                                                 nullptr,
                                                 nullptr,
                                                 G_DBUS_CALL_FLAGS_NONE,
                                                 30000,  // 30 second timeout
                                                 nullptr,
                                                 &error);

  if (!result)
  {
    if (error)
    {
      std::cerr << "Failed to connect to device: " << error->message
                << std::endl;
      g_error_free(error);
    }
    return false;
  }

  g_variant_unref(result);

  // Wait for connection to be established
  for (int i = 0; i < 50; ++i)
  {
    update_properties();
    if (connected_)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return connected_;
}

bool BluetoothDevice::disconnect()
{
  if (!connection_ || !connected_)
    return true;

  GError*   error  = nullptr;
  GVariant* result = g_dbus_connection_call_sync(connection_,
                                                 BlueZ::SERVICE_NAME,
                                                 object_path_.c_str(),
                                                 BlueZ::DEVICE_INTERFACE,
                                                 "Disconnect",
                                                 nullptr,
                                                 nullptr,
                                                 G_DBUS_CALL_FLAGS_NONE,
                                                 10000,
                                                 nullptr,
                                                 &error);

  if (!result)
  {
    if (error)
    {
      std::cerr << "Failed to disconnect from device: " << error->message
                << std::endl;
      g_error_free(error);
    }
    return false;
  }

  g_variant_unref(result);

  // Wait for disconnection
  for (int i = 0; i < 30; ++i)
  {
    update_properties();
    if (!connected_)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return !connected_;
}

bool BluetoothDevice::pair()
{
  if (!connection_)
    return false;

  GError*   error  = nullptr;
  GVariant* result = g_dbus_connection_call_sync(connection_,
                                                 BlueZ::SERVICE_NAME,
                                                 object_path_.c_str(),
                                                 BlueZ::DEVICE_INTERFACE,
                                                 "Pair",
                                                 nullptr,
                                                 nullptr,
                                                 G_DBUS_CALL_FLAGS_NONE,
                                                 30000,
                                                 nullptr,
                                                 &error);

  if (!result)
  {
    if (error)
    {
      std::cerr << "Failed to pair with device: " << error->message
                << std::endl;
      g_error_free(error);
    }
    return false;
  }

  g_variant_unref(result);
  return true;
}

bool BluetoothDevice::unpair()
{
  // Note: Unpairing is typically done through the adapter, not the device
  return true;
}

bool BluetoothDevice::refresh_services()
{
  if (!connection_ || !connected_)
    return false;

  // Wait for services to be resolved
  for (int i = 0; i < 100; ++i)
  {
    update_properties();
    if (services_resolved_)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (!services_resolved_)
  {
    Utils::print_with_timestamp(
      "Services not resolved yet, discovering characteristics anyway...");
  }

  discover_services_and_characteristics();
  return !characteristics_.empty();
}

void BluetoothDevice::discover_services_and_characteristics()
{
  if (!connection_)
    return;

  characteristics_.clear();

  // Get all managed objects to find characteristics for this device
  GError*   error = nullptr;
  GVariant* result =
    g_dbus_connection_call_sync(connection_,
                                BlueZ::SERVICE_NAME,
                                "/",
                                BlueZ::OBJECT_MANAGER_INTERFACE,
                                "GetManagedObjects",
                                nullptr,
                                G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                nullptr,
                                &error);

  if (!result)
  {
    if (error)
    {
      std::cerr << "Failed to get managed objects: " << error->message
                << std::endl;
      g_error_free(error);
    }
    return;
  }

  GVariantIter* objects_iter;
  g_variant_get(result, "(a{oa{sa{sv}}})", &objects_iter);

  const gchar* object_path;
  GVariant*    interfaces_dict;

  while (g_variant_iter_loop(
    objects_iter, "{&o@a{sa{sv}}}", &object_path, &interfaces_dict))
  {
    // Check if this object path belongs to our device
    if (g_str_has_prefix(object_path, object_path_.c_str()))
    {
      GVariantIter* interfaces_iter;
      g_variant_get(interfaces_dict, "a{sa{sv}}", &interfaces_iter);

      const gchar* interface_name;
      GVariant*    properties_dict;

      while (g_variant_iter_loop(
        interfaces_iter, "{&s@a{sv}}", &interface_name, &properties_dict))
      {
        if (g_strcmp0(interface_name, BlueZ::GATT_CHARACTERISTIC_INTERFACE) ==
            0)
        {
          auto characteristic =
            std::make_shared<GattCharacteristic>(connection_, object_path);
          characteristics_[object_path] = characteristic;
        }
      }

      g_variant_iter_free(interfaces_iter);
    }
  }

  g_variant_iter_free(objects_iter);
  g_variant_unref(result);

  Utils::print_with_timestamp("Discovered " +
                              std::to_string(characteristics_.size()) +
                              " characteristics");
}

std::vector<std::shared_ptr<GattCharacteristic>>
BluetoothDevice::get_characteristics()
{
  std::vector<std::shared_ptr<GattCharacteristic>> char_list;

  for (const auto& pair : characteristics_)
  {
    char_list.push_back(pair.second);
  }

  return char_list;
}

std::shared_ptr<GattCharacteristic> BluetoothDevice::get_characteristic(
  const std::string& service_uuid,
  const std::string& char_uuid)
{
  for (const auto& pair : characteristics_)
  {
    if (g_ascii_strcasecmp(pair.second->get_uuid().c_str(),
                           char_uuid.c_str()) == 0)
    {
      return pair.second;
    }
  }
  return nullptr;
}

std::shared_ptr<GattCharacteristic> BluetoothDevice::get_characteristic_by_path(
  const std::string& char_path)
{
  auto it = characteristics_.find(char_path);
  if (it != characteristics_.end())
  {
    return it->second;
  }
  return nullptr;
}

bool BluetoothDevice::read_characteristic(const std::string&    service_uuid,
                                          const std::string&    char_uuid,
                                          std::vector<uint8_t>& data)
{
  auto characteristic = get_characteristic(service_uuid, char_uuid);
  if (!characteristic)
  {
    Utils::print_with_timestamp("Characteristic not found: " + char_uuid);
    return false;
  }

  return characteristic->read_value(data);
}

bool BluetoothDevice::write_characteristic(const std::string& service_uuid,
                                           const std::string& char_uuid,
                                           const std::vector<uint8_t>& data)
{
  auto characteristic = get_characteristic(service_uuid, char_uuid);
  if (!characteristic)
  {
    Utils::print_with_timestamp("Characteristic not found: " + char_uuid);
    return false;
  }

  return characteristic->write_value(data);
}

bool BluetoothDevice::subscribe_to_notifications(
  const std::string&   service_uuid,
  const std::string&   char_uuid,
  NotificationCallback callback)
{
  auto characteristic = get_characteristic(service_uuid, char_uuid);
  if (!characteristic)
  {
    Utils::print_with_timestamp("Characteristic not found: " + char_uuid);
    return false;
  }

  return characteristic->start_notifications(callback);
}

bool BluetoothDevice::unsubscribe_from_notifications(
  const std::string& service_uuid,
  const std::string& char_uuid)
{
  auto characteristic = get_characteristic(service_uuid, char_uuid);
  if (!characteristic)
  {
    Utils::print_with_timestamp("Characteristic not found: " + char_uuid);
    return false;
  }

  return characteristic->stop_notifications();
}

void BluetoothDevice::print_device_info()
{
  std::cout << "\n=== Device Information ===" << std::endl;
  std::cout << "Name: " << name_ << std::endl;
  std::cout << "Address: " << address_ << std::endl;
  std::cout << "Object Path: " << object_path_ << std::endl;
  std::cout << "Connected: " << (connected_ ? "Yes" : "No") << std::endl;
  std::cout << "Services Resolved: " << (services_resolved_ ? "Yes" : "No")
            << std::endl;

  if (!service_uuids_.empty())
  {
    std::cout << "Service UUIDs:" << std::endl;
    for (const auto& uuid : service_uuids_)
    {
      std::cout << "  " << uuid << std::endl;
    }
  }

  std::cout << "Characteristics: " << characteristics_.size() << std::endl;
  std::cout << std::endl;
}

void BluetoothDevice::print_services_and_characteristics()
{
  if (characteristics_.empty())
  {
    Utils::print_with_timestamp(
      "No characteristics discovered. Make sure device is connected and "
      "services are resolved.");
    return;
  }

  std::cout << "\n=== Services and Characteristics ===" << std::endl;

  for (const auto& pair : characteristics_)
  {
    const auto& characteristic = pair.second;
    std::cout << "Characteristic: " << characteristic->get_uuid() << std::endl;
    std::cout << "  Path: " << characteristic->get_object_path() << std::endl;
    std::cout << "  Flags: " << characteristic->flags_to_string() << std::endl;
    std::cout << std::endl;
  }
}

bool BluetoothDevice::has_service(const std::string& service_uuid)
{
  for (const auto& uuid : service_uuids_)
  {
    if (g_ascii_strcasecmp(uuid.c_str(), service_uuid.c_str()) == 0)
    {
      return true;
    }
  }
  return false;
}

void BluetoothDevice::update_connection_state(bool connected)
{
  if (connected_ != connected)
  {
    connected_ = connected;
    Utils::print_with_timestamp("Device " + address_ +
                                " connection state changed: " +
                                (connected ? "Connected" : "Disconnected"));

    if (connected && !services_resolved_)
    {
      // Give some time for services to be resolved
      std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        if (connected_)
        {
          refresh_services();
        }
      }).detach();
    }
  }
}

void BluetoothDevice::update_services_resolved_state(bool resolved)
{
  if (services_resolved_ != resolved)
  {
    services_resolved_ = resolved;
    Utils::print_with_timestamp("Device " + address_ + " services resolved: " +
                                (resolved ? "Yes" : "No"));

    if (resolved && connected_)
    {
      discover_services_and_characteristics();
    }
  }
}