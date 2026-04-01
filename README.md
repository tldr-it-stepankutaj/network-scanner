# network-scanner

![GitHub Release](https://img.shields.io/github/v/release/tldr-it-stepankutaj/network-scanner)
![CI](https://github.com/tldr-it-stepankutaj/network-scanner/actions/workflows/build.yml/badge.svg)

`network-scanner` is a blazing-fast network sweep tool written in C++ that supports ICMP (ping), TCP port probing, and fallback modes. It efficiently scans entire IP ranges and provides color-coded RTT output, ideal for sysadmins, red teamers, or infrastructure testers.

## Features

- ICMP scan (raw socket ping)
- TCP fallback (e.g., scan port 22/80/443 if ICMP is blocked)
- RTT-based color output
- Multithreaded (customizable thread count)
- Cross-platform: Linux & macOS (including ARM64)
- JSON output for scripting and automation
- Configurable probe timeout
- MAC vendor identification (via nmap-mac-prefixes database)
- Graceful Ctrl+C handling with partial results
- Automatic subnet detection (supports any CIDR prefix, not just /24)
- Verbose and debug logging modes
- Scan statistics (duration, host count, discovery rate)
- Shell completion for Bash and Zsh
- Man page documentation
- Unit tests via Google Test

## Usage

```bash
network-scanner [options]
```

### Options

| Option | Description |
|--------|-------------|
| `--threads N` | Number of threads (default: CPU cores) |
| `--mode MODE` | Scan mode: `icmp`, `tcp`, or `fallback` (default: `fallback`) |
| `--port PORT` | Port for TCP scanning (default: 80) |
| `--timeout MS` | Probe timeout in milliseconds (default: 1000) |
| `--thorough` | Thorough scan mode (higher accuracy, slower) |
| `--json` | Output results as JSON (non-interactive) |
| `--no-color` | Disable colored output (also respects `NO_COLOR` env) |
| `--verbose` | Show informational messages on stderr |
| `--debug` | Show debug messages on stderr |
| `--show-all` | Show all hosts including unconfirmed ones |
| `--skip-scan` | Skip network scanning |
| `--no-banner` | Disable ASCII art banner |
| `--no-clear` | Don't clear the screen at start |
| `--help` | Display help message |
| `--version` | Display version information |

### Examples

```bash
# Interactive scan with defaults
network-scanner

# TCP scan on port 443 with 64 threads
network-scanner --mode tcp --port 443 --threads 64

# JSON output for scripting
network-scanner --json --no-color | jq .

# Thorough scan with longer timeout
network-scanner --thorough --timeout 3000

# Debug mode to see what's happening
network-scanner --debug --no-banner --no-clear
```

### Shell Completion

The tool comes with shell completion support for both Bash and Zsh. After installation, shell completion will automatically be available for:

- Command options (`--threads`, `--mode`, `--port`, `--timeout`, `--json`, etc.)
- Mode values (`icmp`, `tcp`, `fallback`)

```bash
# View man page
man network-scanner

# View command help
network-scanner --help
```

## Dependencies

### Runtime Dependencies
- libcurl
- libstdc++ (C++ Standard Library) — statically linked in release binaries

### Optional Dependencies
- `fping` - used in fallback mode if raw sockets aren't available (install with `apt`, `yum`, or `brew`)
- `nmap-mac-prefixes` - MAC vendor database at `/usr/share/nmap/nmap-mac-prefixes` for device vendor identification

## Compatibility

### Tested Operating Systems
- Ubuntu 20.04 LTS and newer
- Debian 10 and newer
- CentOS/RHEL 7 and newer (GLIBCXX issue fixed in v2.0.0 via static linking)
- Fedora 33 and newer
- Oracle Linux 9.5
- macOS 12 (Monterey) and newer (Intel & Apple Silicon)

### Required Permissions
- ICMP mode requires root privileges to create raw sockets
- TCP mode can be run as a non-privileged user

## Download

Pre-built binaries are available for Linux and macOS:

### Latest Release

| Platform | Package | Link |
|----------|---------|------|
| Linux (Debian/Ubuntu) | DEB | [network-scanner-latest.deb](https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest) |
| Linux (RHEL/CentOS/Fedora) | RPM | [network-scanner-latest.rpm](https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest) |
| Linux (Generic) | TAR.GZ | [network-scanner-linux.tar.gz](https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest/download/network-scanner-linux.tar.gz) |
| macOS (Apple Silicon) | TAR.GZ | [network-scanner-macos-arm64.tar.gz](https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest/download/network-scanner-macos-arm64.tar.gz) |

### Installation from Binary Archive

```bash
# Download the latest release (Linux)
curl -LO https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest/download/network-scanner-linux.tar.gz

# Extract
tar -xzf network-scanner-linux.tar.gz

# Install (requires root privileges)
sudo cp -r usr/* /usr/

# Verify installation
network-scanner --version
```

## Build

### Prerequisites

- CMake >= 3.10
- C++17 compiler (GCC / Clang)
- libcurl development headers
- macOS or Linux

### Build Dependencies

- `cmake` (>= 3.10)
- `g++` or `clang++` with C++17 support
- `make`
- `libcurl-dev` / `libcurl4-openssl-dev` (Debian/Ubuntu) or `libcurl-devel` (RHEL/Fedora)
- For developing/building RPM: `rpm-build`
- For developing/building DEB: `dpkg-dev`

### Build instructions

#### Using configure script (recommended for most users)

```bash
git clone https://github.com/tldr-it-stepankutaj/network-scanner.git
cd network-scanner
./configure
make
```

You can customize the build with options:
```bash
./configure --prefix=/usr/local --debug
```

Run `./configure --help` to see all available options.

#### Using CMake directly

```bash
git clone https://github.com/tldr-it-stepankutaj/network-scanner.git
cd network-scanner
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### Static linking (for maximum compatibility)

```bash
cmake -B build -DSTATIC_LIBCXX=ON
cmake --build build --parallel
```

### Running tests

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build --parallel
cd build && ctest --output-on-failure
```

## Install

### From Source

If you used the configure script:
```bash
sudo make install
```

If you used CMake directly:
```bash
# From the build directory
sudo make install
```

You can also uninstall the software with:
```bash
sudo make uninstall
```

This will install:
- The main executable to `/usr/bin/`
- Man page to `/usr/share/man/man1/`
- Bash completion to `/usr/share/bash-completion/completions/`
- Zsh completion to `/usr/share/zsh/site-functions/`

To change the installation prefix, use:
```bash
./configure --prefix=/usr/local
```

### From Package
```bash
# Debian/Ubuntu
sudo dpkg -i network-scanner-*.deb

# RHEL/CentOS/Fedora
sudo rpm -i network-scanner-*.rpm
```

## Changelog (v2.0.0)

- **JSON output** (`--json`) for scripting and automation
- **Configurable timeout** (`--timeout MS`) for slow networks
- **`--no-color` flag** and `NO_COLOR` environment variable support
- **`--verbose` / `--debug`** logging to stderr
- **Graceful Ctrl+C** — interrupt scans cleanly with partial results
- **Automatic subnet detection** — uses actual subnet mask instead of hardcoded /24
- **MAC vendor identification** — looks up device manufacturer via ARP + nmap-mac-prefixes
- **Scan statistics** — duration, host count, discovery rate displayed after scan
- **macOS gateway detection** — fixed; previously returned empty on macOS
- **Security fix** — replaced `std::system()` with `fork/exec` in fping fallback (no shell injection)
- **Static libstdc++ linking** — fixes `GLIBCXX_3.4.30` error on older Linux distros (RHEL 9, Oracle Linux)
- **macOS CI builds** — ARM64 binaries now included in releases
- **Unit tests** — Google Test suite for IP/CIDR parsing, checksum, and subnet functions

## License

GPL-3.0 license

---
## Author

This tool was developed by Stepan Kutaj (TLDR-IT). For more information or questions, contact me at [stepan.kutaj@tldr-it.com](mailto:stepan.kutaj@tldr-it.com) or visit my website at [www.tldr-it.com](https://www.tldr-it.com).

[![GitHub](https://img.shields.io/github/followers/tldr-it-stepankutaj?label=Follow%20%40tldr-it-stepankutaj&style=social)](https://github.com/tldr-it-stepankutaj)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?style=social&logo=linkedin)](https://www.linkedin.com/in/stepankutaj)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Support-orange?style=social&logo=buy-me-a-coffee)](https://buymeacoffee.com/stepankutae)

*For educational and legitimate security research purposes only. Always obtain proper authorization before performing reconnaissance on any systems.*
