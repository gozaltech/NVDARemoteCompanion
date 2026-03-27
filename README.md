# NVDA Remote Companion

A cross-platform C++ client for connecting to NVDA Remote servers, enabling remote screen reader access and keyboard input forwarding. This project provides a lightweight, native companion application for NVDA Remote connections.

## Features

### Core Functionality
- **Cross-platform compatibility**: Native support for Windows and Linux
- **Multiple simultaneous connections**: Connect to multiple NVDA Remote servers with per-profile keyboard shortcuts
- **Configuration file**: JSON-based config with profiles, auto-detected or specified via `--config`
- **Interactive CLI**: Manage profiles and connections at runtime (connect, disconnect, add, edit, delete)
- **Background mode** (Windows): Run without a console window, with a system tray icon

### Input and Output
- **Keyboard input forwarding**: Captures and forwards keyboard events to remote sessions (Windows only)
- **Per-profile toggle shortcuts**: Each connection profile has its own shortcut to activate key forwarding
- **Speech synthesis**: Text-to-speech output using SRAL (Screen Reader Abstraction Library)
- **Audio feedback**: Tones indicate when key forwarding is toggled
- **Real-time communication**: Low-latency message handling for responsive remote control

### Development and Deployment
- **Modern C++ implementation**: C++17 standard with modern practices
- **CMake build system**: Cross-platform build configuration with automatic dependency management
- **Automated CI/CD**: GitHub Actions workflow for building releases across all supported platforms

## Supported Platforms and Architectures

### Windows
- **x64 (64-bit Intel/AMD)**

### Linux
- **x64 (64-bit Intel/AMD)**

## Installation

### Download Pre-built Binaries

Download the latest release for your platform from the [Releases](../../releases) page:

- **Windows**: `nvda_remote_companion-windows-{arch}.zip`
- **Linux**: `nvda_remote_companion-linux-{arch}.tar.gz`

Where `{arch}` is `x64`.

### Building from Source

#### Windows (Using Visual Studio with Ninja)

1. **Install Prerequisites**:
   ```cmd
   # Install Visual Studio 2019+ with C++ development tools
   # Install CMake and Git
   ```

2. **Build the project**:
   ```cmd
   cd NVDARemoteCompanion
   mkdir build && cd build
   cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```

3. **Run the executable**:
   ```cmd
   bin\nvda_remote_companion.exe
   ```

#### Linux (Ubuntu/Debian)

1. **Install Prerequisites**:
   ```bash
   sudo apt update
   sudo apt install build-essential cmake git ninja-build
   sudo apt install libspeechd-dev libbrlapi-dev
   ```

2. **Build the project**:
   ```bash
   cd NVDARemoteCompanion
   mkdir build && cd build
   cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```

3. **Run the executable**:
   ```bash
   ./bin/nvda_remote_companion
   ```

## Usage

### Basic Usage

1. **Start the application**:
   ```bash
   ./nvda_remote_companion
   ```

2. **Enter connection details** when prompted (if no config file or CLI args provided):
   - Server host (IP address or domain name)
   - Server port (typically 6837)
   - Connection key (channel identifier)

3. **Control the session**:
   - **Windows**: Toggle keyboard forwarding with the configured shortcut (default: Ctrl+Win+F11)
   - **Linux**: Currently operates in receive-only mode
   - Speech from the remote session will be played locally
   - Use interactive commands to manage connections (type `help` at the `>` prompt)
   - Press `Ctrl+C` or type `quit` to exit

### Command Line Options

```
./nvda_remote_companion [OPTIONS]

Connection Options:
  -h, --host HOST       Server hostname or IP address
  -p, --port PORT       Server port (default: 6837)
  -k, --key KEY         Connection key/channel
  -s, --shortcut KEY    Set toggle shortcut (default: ctrl+win+f11)

Debug Options:
  -d, --debug           Enable debug logging (INFO level)
  -v, --verbose         Enable verbose debug logging
  -t, --trace           Enable trace debug logging (most detailed)

Config File Options:
  -c, --config PATH     Path to config file (default: auto-detect)
      --create-config   Create a default config file and exit

Other Options:
      --no-speech       Disable speech synthesis
  -b, --background      Run without console window, system tray only (Windows)
      --help            Show help message

Examples:
  ./nvda_remote_companion -h example.com -k mykey
  ./nvda_remote_companion --host 192.168.1.100 --port 6837 --key shared_session
  ./nvda_remote_companion --config myconfig.json
  ./nvda_remote_companion --background
```

### Configuration File

The application looks for `nvdaremote.json` in:
1. Current directory
2. `%APPDATA%\NVDARemoteCompanion\` (Windows) or `~/.config/NVDARemoteCompanion/` (Linux)

Generate a default config file with:
```bash
./nvda_remote_companion --create-config
```

#### Config File Format

```json
{
    "debug_level": "warning",
    "speech": true,
    "background": false,
    "profiles": [
        {
            "name": "work-pc",
            "host": "remote.example.com",
            "port": 6837,
            "key": "my-work-key",
            "shortcut": "ctrl+win+f11",
            "auto_connect": true
        },
        {
            "name": "home-pc",
            "host": "home.example.com",
            "port": 6837,
            "key": "my-home-key",
            "shortcut": "ctrl+win+f12",
            "auto_connect": false
        }
    ]
}
```

#### Global Settings

| Field | Type | Description |
|-------|------|-------------|
| `debug_level` | string | Logging level: `"warning"`, `"info"`, `"verbose"`, `"trace"` |
| `speech` | bool | Enable/disable speech synthesis |
| `background` | bool | Run in background mode with system tray (Windows only) |
| `profiles` | array | Connection profiles (see below) |

#### Profile Fields

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `name` | string | No | hostname | Display name for the profile |
| `host` | string | Yes | | Server hostname or IP address |
| `port` | int | No | 6837 | Server port |
| `key` | string | Yes | | Connection key/channel |
| `shortcut` | string | No | ctrl+win+f11 | Toggle shortcut for keyboard forwarding |
| `auto_connect` | bool | No | true | Connect automatically on startup |

Command-line arguments override config file values. When using `--host`/`--key` on the command line, a single ad-hoc profile is created and config file profiles are ignored.

### Interactive Commands

While the application is running, you can manage profiles and connections from the `>` prompt:

| Command | Alias | Description |
|---------|-------|-------------|
| `status` | | Show all profiles with connection status |
| `list` | `ls` | List all profiles with full details |
| `connect [name\|index]` | `c` | Connect a specific profile, or all disconnected profiles |
| `disconnect <name\|index>` | `dc` | Disconnect a specific profile |
| `add <name> <host> <key> [port] [shortcut] [auto_connect]` | | Add a new profile |
| `edit <name\|index> <field> <value>` | | Edit a profile field |
| `delete <name\|index>` | `rm` | Delete a profile |
| `help` | `?` | Show available commands |
| `quit` | `exit` | Exit the application |

Profiles can be referenced by name or index number. Changes made with `add`, `edit`, and `delete` are saved to the config file immediately.

#### Examples

```
> status
Profiles: 2 total, 1 connected
Config: nvdaremote.json
  [0] work-pc - remote.example.com:6837 - CONNECTED
  [1] home-pc - home.example.com:6837 - DISCONNECTED (manual)

> connect home-pc
Connecting to home-pc (home.example.com:6837)...
Connected to home-pc

> disconnect 0
Disconnecting work-pc...
Disconnected work-pc

> add server3 192.168.1.50 mykey 6837 ctrl+win+f10
Profile added: [2] server3

> edit server3 auto_connect false
Profile [2] server3 updated: auto_connect = false

> delete server3
Profile 'server3' deleted
```

### Multiple Connections

Each profile connects to a separate NVDA Remote server/channel simultaneously. On Windows, each profile has its own toggle shortcut to activate keyboard forwarding to that specific connection.

- Press a profile's shortcut to start sending keys to that remote machine
- Press the same shortcut again to stop
- Press a different profile's shortcut to switch key forwarding to that machine (keys are released on the previous one automatically)
- Disconnected profiles have their shortcuts disabled automatically
- Use `connect` / `disconnect` commands to manage connections at runtime

### Background Mode (Windows)

Background mode runs the application without a console window. A system tray icon provides an exit option.

```bash
# Via command line
nvda_remote_companion.exe --background --host example.com --key mykey

# Via config file (set "background": true)
nvda_remote_companion.exe --config nvdaremote.json
```

Background mode requires connection parameters (via CLI or config file) since there is no console for interactive input. Interactive commands are not available in background mode.

## Limitations

### Current Limitations

#### Linux Platform
- **No keyboard input forwarding**: Linux implementation currently supports receive-only mode
- **No background mode**: Background mode with system tray is Windows only

## Contributing

We welcome contributions to improve NVDA Remote Companion:

### Development Guidelines
1. **Code style**: Follow existing C++17 conventions and formatting
2. **Threading**: Ensure thread safety for all multi-threaded components
3. **Testing**: Test on both Windows and Linux when possible
4. **Documentation**: Update relevant documentation for new features
5. **Logging**: Add appropriate debug logging for troubleshooting

### Areas for Contribution
- **Linux keyboard input forwarding**: Implementation of system-wide input capture
- **Additional speech engines**: Integration with more TTS systems
- **Protocol enhancements**: Support for additional NVDA Remote features
- **Performance optimization**: Memory usage and connection reliability improvements
- **Platform support**: macOS port or additional Linux distributions

## Support

### Donation

If you find NVDA Remote Companion helpful, consider supporting its development:

[![PayPal](https://img.shields.io/badge/PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/gozaltech)

Your support helps maintain and improve this project for the accessibility community.

### Contact

For questions, suggestions, or technical support:

[![Telegram](https://img.shields.io/badge/Telegram-2CA5E0?style=for-the-badge&logo=telegram&logoColor=white)](https://t.me/beqabeqa483)

You can also:
- [Open an issue](../../issues) for bug reports or feature requests
- [Start a discussion](../../discussions) for general questions
- [Submit a pull request](../../pulls) for contributions
