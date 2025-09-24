# bscm-gdbus-cpp

A comprehensive C++ application that uses BlueZ with the GDBus API to scan for Bluetooth devices, connect to them, and manage GATT characteristics with the ability to subscribe to GATT notifications.

## Features

- **Device Discovery**: Scan for Bluetooth devices with optional service UUID filtering
- **Connection Management**: Connect to and disconnect from Bluetooth devices
- **GATT Operations**: Read from and write to GATT characteristics
- **Notifications**: Subscribe to GATT characteristic notifications with real-time callbacks
- **Command-Line Interface**: Interactive CLI for easy device management
- **Real-time Processing**: Live notification display with timestamps

## Prerequisites

### System Requirements
- Linux system with BlueZ stack
- GLib 2.0 and GIO development libraries
- CMake 3.16 or later
- C++17 compatible compiler

### Installing Dependencies

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential cmake libglib2.0-dev libdbus-1-dev
```

On Fedora/RHEL:
```bash
sudo dnf install gcc-c++ cmake glib2-devel dbus-devel
```

### BlueZ Setup
Ensure BlueZ is installed and the bluetooth service is running:
```bash
sudo systemctl enable bluetooth
sudo systemctl start bluetooth
```

## Building

1. Clone the repository:
```bash
git clone https://github.com/WeekendWar/bscm-gdbus-cpp.git
cd bscm-gdbus-cpp
```

2. Create build directory and compile:
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

3. The executable will be created at `build/bscm-gdbus-cpp`

## Usage

### Running the Application

```bash
./bscm-gdbus-cpp
```

### Command Reference

| Command | Description | Example |
|---------|-------------|---------|
| `help` | Show all available commands | `help` |
| `power on/off` | Control Bluetooth adapter power | `power on` |
| `scan [service_uuid]` | Start device discovery | `scan` or `scan 0000180f-0000-1000-8000-00805f9b34fb` |
| `stop` | Stop device discovery | `stop` |
| `list` | List discovered devices | `list` |
| `connect <address>` | Connect to device | `connect AA:BB:CC:DD:EE:FF` |
| `disconnect` | Disconnect current device | `disconnect` |
| `services` | List services and characteristics | `services` |
| `read <service_uuid> <char_uuid>` | Read characteristic value | `read 0000180f-0000-1000-8000-00805f9b34fb 00002a19-0000-1000-8000-00805f9b34fb` |
| `write <service_uuid> <char_uuid> <hex_data>` | Write to characteristic | `write 0000180f-0000-1000-8000-00805f9b34fb 00002a19-0000-1000-8000-00805f9b34fb 01FF` |
| `notify <service_uuid> <char_uuid> [on/off]` | Enable/disable notifications | `notify 0000180f-0000-1000-8000-00805f9b34fb 00002a19-0000-1000-8000-00805f9b34fb on` |
| `device` | Show current device information | `device` |
| `quit/exit` | Exit the application | `quit` |

### Example Session

```bash
$ ./bscm-gdbus-cpp
[2025-01-24 10:30:15.123] Bluetooth GATT Client starting...
[2025-01-24 10:30:15.456] Bluetooth manager initialized
[2025-01-24 10:30:15.789] Using adapter: /org/bluez/hci0

=== Bluetooth GATT Client Commands ===
  help                        - Show this help message
  quit/exit                   - Exit the application
  power on/off                - Power on/off the Bluetooth adapter
  scan [service_uuid]         - Start scanning for devices
  ...

bt> power on
[2025-01-24 10:30:20.123] Adapter powered on

bt> scan
[2025-01-24 10:30:25.456] Started scanning for all devices
[2025-01-24 10:30:26.789] Device discovered: MyDevice (AA:BB:CC:DD:EE:FF)

bt> connect AA:BB:CC:DD:EE:FF
[2025-01-24 10:30:30.123] Connecting to MyDevice (AA:BB:CC:DD:EE:FF)...
[2025-01-24 10:30:32.456] Connected successfully!
[2025-01-24 10:30:34.789] Services discovered

bt> services
=== Services and Characteristics ===
Characteristic: 00002a19-0000-1000-8000-00805f9b34fb
  Path: /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service001a/char001b
  Flags: read, notify

bt> notify 0000180f-0000-1000-8000-00805f9b34fb 00002a19-0000-1000-8000-00805f9b34fb on
[2025-01-24 10:30:40.123] Notifications enabled for 00002a19-0000-1000-8000-00805f9b34fb
[2025-01-24 10:30:45.456] NOTIFICATION [00002a19-0000-1000-8000-00805f9b34fb]: 64
[2025-01-24 10:30:50.789] NOTIFICATION [00002a19-0000-1000-8000-00805f9b34fb]: 63
```

## Architecture

### Core Components

- **BluetoothManager**: Manages the Bluetooth adapter, device discovery, and D-Bus connections
- **BluetoothDevice**: Represents individual Bluetooth devices and handles connections
- **GattCharacteristic**: Manages GATT characteristic operations (read/write/notify)
- **NotificationHandler**: Handles D-Bus signals for GATT characteristic notifications
- **CLI Interface**: Provides an interactive command-line interface

### D-Bus Integration

The application communicates with BlueZ through the D-Bus system bus using the following interfaces:
- `org.bluez.Adapter1` - Bluetooth adapter management
- `org.bluez.Device1` - Device connection and properties
- `org.bluez.GattCharacteristic1` - GATT operations
- `org.freedesktop.DBus.Properties` - Property change notifications

## Common Service UUIDs

| Service | UUID | Description |
|---------|------|-------------|
| Battery Service | `0000180f-0000-1000-8000-00805f9b34fb` | Battery level information |
| Device Information | `0000180a-0000-1000-8000-00805f9b34fb` | Device manufacturer, model, etc. |
| Heart Rate | `0000180d-0000-1000-8000-00805f9b34fb` | Heart rate measurements |
| Nordic UART | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` | Nordic UART service |

## Troubleshooting

### Permission Issues
If you encounter D-Bus permission errors, ensure your user is in the `bluetooth` group:
```bash
sudo usermod -a -G bluetooth $USER
```
Then log out and log back in.

### BlueZ Not Running
If the application fails to connect to D-Bus:
```bash
sudo systemctl status bluetooth
sudo systemctl start bluetooth
```

### Device Connection Issues
- Ensure the device is in pairing/discoverable mode
- Check that the device isn't connected to another system
- Try removing the device and re-pairing: `bluetoothctl remove <address>`

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Author

Gean Johnson - WeekendWar
