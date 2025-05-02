# network-scanner

![GitHub Release](https://img.shields.io/github/v/release/tldr-it-stepankutaj/network-scanner)
![CI](https://github.com/tldr-it-stepankutaj/network-scanner/actions/workflows/build.yml/badge.svg)

`network-scanner` is a blazing-fast network sweep tool written in C++ that supports ICMP (ping), TCP port probing, and fallback modes. It efficiently scans entire IP ranges and provides color-coded RTT output, ideal for sysadmins, red teamers, or infrastructure testers.

## ✨ Features

- ✅ ICMP scan (raw socket ping)
- ✅ TCP fallback (e.g., scan port 22/80/443 if ICMP is blocked)
- ✅ RTT-based color output
- ✅ Multithreaded (customizable thread count)
- ✅ Cross-platform: Linux & macOS
- ✅ Shell completion for Bash and Zsh
- ✅ Man page documentation

## 🚀 Usage

```bash
network-scanner [--threads N] [--mode icmp|tcp|fallback] [--port PORT]
```

### Example

```bash
network-scanner --mode fallback --port 443 --threads 64
```

### Shell Completion

The tool comes with shell completion support for both Bash and Zsh. After installation, shell completion will automatically be available for:

- Command options (`--threads`, `--mode`, `--port`, etc.)
- Mode values (`icmp`, `tcp`, `fallback`)

You can access help information using:

```bash
# View man page
man network-scanner

# View command help
network-scanner --help
```

## 📋 Dependencies

### Runtime Dependencies
- libstdc++ (C++ Standard Library)
- glibc (≥ 2.17)

### Optional Dependencies
- `fping` - used in fallback mode if raw sockets aren't available (install with `apt`, `yum`, or `brew`)

## 💻 Compatibility

### Tested Operating Systems
- ✅ Ubuntu 20.04 LTS and newer
- ✅ Debian 10 and newer
- ✅ CentOS/RHEL 7 and newer
- ✅ Fedora 33 and newer
- ✅ macOS 10.15 (Catalina) and newer

### Known Issues
- ❗ RedHat (Oracle Linux) 9.5: `network-scanner: /lib64/libstdc++.so.6: version 'GLIBCXX_3.4.30' not found (required by network-scanner)`

### Required Permissions
- ICMP mode requires root privileges to create raw sockets
- TCP mode can be run as a non-privileged user

## 📥 Download

Pre-built binaries are available for Linux systems:

### Latest Release

| Platform | Package | Link |
|----------|---------|------|
| Linux (Debian/Ubuntu) | DEB | [network-scanner-latest.deb](https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest/download/network-scanner-*.deb) |
| Linux (RHEL/CentOS/Fedora) | RPM | [network-scanner-latest.rpm](https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest/download/network-scanner-*.rpm) |
| Linux (Generic) | TAR.GZ | [network-scanner-linux.tar.gz](https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest/download/network-scanner-linux.tar.gz) |

### Installation from Binary Archive

For the generic Linux package (tar.gz):

```bash
# Download the latest release
curl -LO https://github.com/tldr-it-stepankutaj/network-scanner/releases/latest/download/network-scanner-linux.tar.gz

# Extract
tar -xzf network-scanner-linux.tar.gz

# Install (requires root privileges)
sudo cp -r usr/* /usr/

# Verify installation
network-scanner --version
```

## 🔧 Build

### Prerequisites

- CMake ≥ 3.10
- C++17 compiler (GCC / Clang)
- macOS or Linux

### Build Dependencies

- `cmake` (≥ 3.10)
- `g++` or `clang++` with C++17 support
- `make`
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

## 📦 Install

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

## 🛡 License

GPL-3.0 license

---
## Author

This tool was developed by Stepan Kutaj (TLDR-IT). For more information or questions, contact me at [stepan.kutaj@tldr-it.com](mailto:stepan.kutaj@tldr-it.com) or visit my website at [www.tldr-it.com](https://www.tldr-it.com).

[![GitHub](https://img.shields.io/github/followers/tldr-it-stepankutaj?label=Follow%20%40tldr-it-stepankutaj&style=social)](https://github.com/tldr-it-stepankutaj)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?style=social&logo=linkedin)](https://www.linkedin.com/in/stepankutaj)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Support-orange?style=social&logo=buy-me-a-coffee)](https://buymeacoffee.com/stepankutae)

*For educational and legitimate security research purposes only. Always obtain proper authorization before performing reconnaissance on any systems.*