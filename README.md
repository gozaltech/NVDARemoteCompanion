# NVDA Remote Companion

A cross-platform C++ client for connecting to NVDA Remote servers, enabling remote screen reader access and keyboard input forwarding. This project provides a lightweight, native companion application for NVDA Remote connections.

## Features

### Core Functionality
- **Cross-platform compatibility**: Native support for Windows and Linux

### Input and Output
- **Keyboard input forwarding**: Captures and forwards keyboard events to remote sessions (Windows only)
- **Speech synthesis**: Text-to-speech output using SRAL (Screen Reader Abstraction Layer)
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

2. **Enter connection details** when prompted:
   - Server host (IP address or domain name)
   - Server port (typically 6837)
   - Connection key (channel identifier)

3. **Control the session**:
   - **Windows**: Keyboard input will be captured and forwarded to the remote session by pressing ctrl+windows+f11
   - **Linux**: Currently operates in receive-only mode
   - Speech from the remote session will be played locally
   - Press `Ctrl+C` to exit gracefully

### Command Line Options

```bash
./nvda_remote_companion [options]

Connection Options:
  -h, --host HOST       Server hostname or IP address
  -p, --port PORT       Server port (default: 6837)  
  -k, --key KEY         Connection key/channel

Debug Options:
  -d, --debug           Enable debug logging (INFO level)
  -v, --verbose         Enable verbose debug logging
  -t, --trace           Enable trace debug logging (most detailed)

Other Options:
      --no-speech       Disable speech synthesis
      --help            Show help message

Examples:
  ./nvda_remote_companion -h example.com -k mykey
  ./nvda_remote_companion --host 192.168.1.100 --port 6837 --key shared_session
  ./nvda_remote_companion --verbose --no-speech
```

## Limitations

### Current Limitations

#### Linux Platform
- **No keyboard input forwarding**: Linux implementation currently supports receive-only mode

#### General Limitations
- **Single connection**: Only one active connection supported at a time

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
- **Multiple connections**: maintain multiple simultaneous connections and switch between them with a shortcut

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
