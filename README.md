# network-scanner

![GitHub release](https://img.shields.io/github/v/release/tldr-it-stepankutaj/network-scanner)
![CI](https://github.com/tldr-it-stepankutaj/network-scanner/actions/workflows/build.yml/badge.svg)

`network-scanner` is a blazing-fast network sweep tool written in C++ that supports ICMP (ping), TCP port probing, and fallback modes. It efficiently scans entire IP ranges and provides color-coded RTT output, ideal for sysadmins, red teamers, or infrastructure testers.

## ✨ Features

- ✅ CIDR input (e.g., `192.168.0.0/24`, `10.0.0.0/16`)
- ✅ ICMP scan (raw socket ping)
- ✅ TCP fallback (e.g., scan port 22/80/443 if ICMP is blocked)
- ✅ RTT-based color output
- ✅ Multithreaded (customizable thread count)
- ✅ Cross-platform: Linux & macOS

## 🚀 Usage

```bash
network-scanner <CIDR> [--threads N] [--mode icmp|tcp|fallback] [--port PORT]
```

### Example

```bash
network-scanner 192.168.1.0/24 --mode fallback --port 443 --threads 64
```

## 🔧 Build

### Prerequisites

- CMake ≥ 3.10
- C++17 compiler (GCC / Clang)
- macOS or Linux

### Build instructions

```bash
git clone https://github.com/stepankutaj/network-scanner.git
cd network-scanner
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 📦 Install

```bash
sudo make install
```

Manpage and shell completions will also be installed if available.

## 🛡 License

MIT License

---

> Created by [Stepan Kutaj](https://github.com/stepankutaj)
