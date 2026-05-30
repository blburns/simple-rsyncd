# Build Guide

Complete guide to building Simple RSync Daemon on different platforms.

## Quick Start

```bash
# Clone repository
git clone https://github.com/SimpleDaemons/simple-rsyncd.git
cd simple-rsyncd

# Build
mkdir build && cd build
cmake ..
make

# Run
./simple-rsyncd --help
```

---

## Build Commands Reference

### Basic Build

```bash
# Configure
cmake ..

# Build
make

# Install (requires root)
sudo make install
```

### Optional CMake Flags

```bash
cmake -DENABLE_TESTS=ON ..      # Build and register tests (default: ON)
cmake -DENABLE_SSL=OFF ..       # Disable TLS support
cmake -DENABLE_JSON=OFF ..      # Disable JSON configuration support
cmake -DENABLE_YAML=OFF ..      # Disable YAML configuration support
cmake -DENABLE_PACKAGING=OFF .. # Skip CPack package generation
```

---

**Last Updated:** December 2024
