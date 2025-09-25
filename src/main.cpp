#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "BluetoothManager.h"
#include "Common.h"

class BluetoothCLI
{
private:
  BluetoothManager                 manager_;
  std::shared_ptr<BluetoothDevice> current_device_;
  GMainLoop*                       main_loop_;

  void print_help()
  {
    std::cout
      << std::endl
      << "=== Bluetooth GATT Client Commands ===" << std::endl
      << "  help                        - Show this help message" << std::endl
      << "  quit/exit                   - Exit the application" << std::endl

      << "  power on/off                - Power on/off the Bluetooth adapter"
      << std::endl
      << "  scan [service_uuid]         - Start scanning for devices"
      << std::endl
      << "                                (optionally filter by service UUID)"
      << std::endl
      << "  stop                        - Stop scanning" << std::endl
      << "  list                        - List discovered devices" << std::endl
      << "  connect <address>           - Connect to device by MAC address"
      << std::endl

      << "  disconnect                  - Disconnect from current device"
      << std::endl
      << "  services                    - List services and characteristics"
         " of connected device"
      << std::endl
      << "  read <service_uuid> <char_uuid>  - Read characteristic value"
      << std::endl
      << "  write <service_uuid> <char_uuid> <hex_data>  - Write to "
         "characteristic"
      << std::endl
      << "  notify <service_uuid> <char_uuid> [on/off]   - "
         "Enable/disable notifications"
      << std::endl
      << "  device                      - Show current device info" << std::endl
      << std::endl;
  }

  std::vector<std::string> split_string(const std::string& str, char delimiter)
  {
    std::vector<std::string> tokens;
    std::stringstream        ss(str);
    std::string              token;

    while (std::getline(ss, token, delimiter))
    {
      if (!token.empty())
      {
        tokens.push_back(token);
      }
    }

    return tokens;
  }

  void handle_scan_command(const std::vector<std::string>& args)
  {
    if (args.size() > 1)
    {
      std::vector<std::string> service_uuids = {args[1]};
      manager_.set_target_service_uuids(service_uuids);
      if (manager_.start_discovery(service_uuids))
      {
        Utils::print_with_timestamp(
          "Started scanning for devices with service UUID: " + args[1]);
      }
      else
      {
        Utils::print_with_timestamp("Failed to start scanning");
      }
    }
    else
    {
      manager_.set_target_service_uuids({});
      if (manager_.start_discovery())
      {
        Utils::print_with_timestamp("Started scanning for all devices");
      }
      else
      {
        Utils::print_with_timestamp("Failed to start scanning");
      }
    }
  }

  void handle_connect_command(const std::vector<std::string>& args)
  {
    if (args.size() < 2)
    {
      std::cout << "Usage: connect <mac_address>" << std::endl;
      return;
    }

    auto device = manager_.get_device(args[1]);
    if (!device)
    {
      Utils::print_with_timestamp("Device not found: " + args[1]);
      return;
    }

    Utils::print_with_timestamp("Connecting to " + device->get_name() + " (" +
                                args[1] + ")...");
    if (device->connect())
    {
      current_device_ = device;
      Utils::print_with_timestamp("Connected successfully!");

      // Wait a moment for services to be resolved
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));

      if (device->refresh_services())
      {
        Utils::print_with_timestamp("Services discovered");
      }
    }
    else
    {
      Utils::print_with_timestamp("Failed to connect");
    }
  }

  void handle_read_command(const std::vector<std::string>& args)
  {
    if (!current_device_ || !current_device_->is_connected())
    {
      Utils::print_with_timestamp("No device connected");
      return;
    }

    if (args.size() < 3)
    {
      std::cout << "Usage: read <service_uuid> <characteristic_uuid>"
                << std::endl;
      return;
    }

    std::vector<uint8_t> data;
    if (current_device_->read_characteristic(args[1], args[2], data))
    {
      Utils::print_with_timestamp(
        "Read " + std::to_string(data.size()) +
        " bytes: " + Utils::bytes_to_hex_string(data));
    }
    else
    {
      Utils::print_with_timestamp("Failed to read characteristic");
    }
  }

  void handle_write_command(const std::vector<std::string>& args)
  {
    if (!current_device_ || !current_device_->is_connected())
    {
      Utils::print_with_timestamp("No device connected");
      return;
    }

    if (args.size() < 4)
    {
      std::cout
        << "Usage: write <service_uuid> <characteristic_uuid> <hex_data>"
        << std::endl;
      return;
    }

    auto data = Utils::hex_string_to_bytes(args[3]);
    if (current_device_->write_characteristic(args[1], args[2], data))
    {
      Utils::print_with_timestamp("Write successful: " +
                                  Utils::bytes_to_hex_string(data));
    }
    else
    {
      Utils::print_with_timestamp("Failed to write characteristic");
    }
  }

  void handle_notify_command(const std::vector<std::string>& args)
  {
    if (!current_device_ || !current_device_->is_connected())
    {
      Utils::print_with_timestamp("No device connected");
      return;
    }

    if (args.size() < 3)
    {
      std::cout << "Usage: notify <service_uuid> <characteristic_uuid> [on/off]"
                << std::endl;
      return;
    }

    bool enable = true;
    if (args.size() >= 4)
    {
      enable = (args[3] == "on" || args[3] == "true" || args[3] == "1");
    }

    if (enable)
    {
      auto callback = [this, args](const std::string&          char_path,
                                   const std::vector<uint8_t>& data) {
        Utils::print_with_timestamp("NOTIFICATION [" + args[2] +
                                    "]: " + Utils::bytes_to_hex_string(data));
      };

      if (current_device_->subscribe_to_notifications(
            args[1], args[2], callback))
      {
        Utils::print_with_timestamp("Notifications enabled for " + args[2]);
      }
      else
      {
        Utils::print_with_timestamp("Failed to enable notifications");
      }
    }
    else
    {
      if (current_device_->unsubscribe_from_notifications(args[1], args[2]))
      {
        Utils::print_with_timestamp("Notifications disabled for " + args[2]);
      }
      else
      {
        Utils::print_with_timestamp("Failed to disable notifications");
      }
    }
  }

public:
  BluetoothCLI() : main_loop_(nullptr) {}

  ~BluetoothCLI()
  {
    if (main_loop_)
    {
      g_main_loop_unref(main_loop_);
    }
  }

  int run()
  {
    Utils::print_with_timestamp("Bluetooth GATT Client starting...");

    if (!manager_.initialize())
    {
      std::cerr << "Failed to initialize Bluetooth manager" << std::endl;
      return 1;
    }

    Utils::print_with_timestamp("Bluetooth manager initialized");

    // Create main loop for GLib events
    main_loop_ = g_main_loop_new(nullptr, FALSE);

    // Start main loop in separate thread
    std::thread loop_thread([this]() { g_main_loop_run(main_loop_); });

    print_help();

    std::string line;
    while (true)
    {
      std::cout << "bt> ";
      if (!std::getline(std::cin, line))
      {
        break;
      }

      // Trim whitespace
      line.erase(0, line.find_first_not_of(" \t"));
      line.erase(line.find_last_not_of(" \t") + 1);

      if (line.empty())
        continue;

      auto args = split_string(line, ' ');
      if (args.empty())
        continue;

      std::string command = args[0];
      std::transform(
        command.begin(), command.end(), command.begin(), ::tolower);

      // print each argument in args
      for (auto arg : args)
      {
        std::cout << "Arg: " << arg << std::endl;
      }

      if (command == "help")
      {
        print_help();
      }
      else if (command == "quit" || command == "exit")
      {
        break;
      }
      else if (command == "power")
      {
        if (args.size() > 1 && args[1] == "on")
        {
          if (manager_.power_on_adapter())
          {
            Utils::print_with_timestamp("Adapter powered on");
          }
          else
          {
            Utils::print_with_timestamp("Failed to power on adapter");
          }
        }
        else if (args.size() > 1 && args[1] == "off")
        {
          if (manager_.power_off_adapter())
          {
            Utils::print_with_timestamp("Adapter powered off");
          }
          else
          {
            Utils::print_with_timestamp("Failed to power off adapter");
          }
        }
        else
        {
          Utils::print_with_timestamp(
            std::string("Adapter is ") +
            (manager_.is_adapter_powered() ? "on" : "off"));
        }
      }
      else if (command == "scan")
      {
        handle_scan_command(args);
      }
      else if (command == "stop")
      {
        if (manager_.stop_discovery())
        {
          Utils::print_with_timestamp("Stopped scanning");
        }
        else
        {
          Utils::print_with_timestamp("Failed to stop scanning");
        }
      }
      else if (command == "list")
      {
        manager_.print_discovered_devices();
      }
      else if (command == "connect")
      {
        handle_connect_command(args);
      }
      else if (command == "disconnect")
      {
        if (current_device_ && current_device_->disconnect())
        {
          Utils::print_with_timestamp("Disconnected");
          current_device_.reset();
        }
        else
        {
          Utils::print_with_timestamp(
            "Failed to disconnect or no device connected");
        }
      }
      else if (command == "services")
      {
        if (current_device_)
        {
          current_device_->print_services_and_characteristics();
        }
        else
        {
          Utils::print_with_timestamp("No device connected");
        }
      }
      else if (command == "read")
      {
        handle_read_command(args);
      }
      else if (command == "write")
      {
        handle_write_command(args);
      }
      else if (command == "notify")
      {
        handle_notify_command(args);
      }
      else if (command == "device")
      {
        if (current_device_)
        {
          current_device_->print_device_info();
        }
        else
        {
          Utils::print_with_timestamp("No device connected");
        }
      }
      else
      {
        std::cout << "Unknown command: " << command
                  << ". Type 'help' for available commands." << std::endl;
      }
    }

    Utils::print_with_timestamp("Shutting down...");

    // Cleanup
    if (current_device_)
    {
      current_device_->disconnect();
    }

    manager_.stop_discovery();
    manager_.cleanup();

    // Stop main loop
    if (main_loop_)
    {
      g_main_loop_quit(main_loop_);
    }

    if (loop_thread.joinable())
    {
      loop_thread.join();
    }

    return 0;
  }
};

int main()
{
  BluetoothCLI cli;
  return cli.run();
}