# Installation Guide

This guide covers the complete installation process for Simple RSync Daemon on all supported platforms.

## 📋 Prerequisites

### System Requirements

- **Operating System**: Linux, macOS 12.0+, or Windows 10+
- **Architecture**: x86_64, ARM64 (Apple Silicon), or ARM64 (Linux)
- **Memory**: Minimum 512MB RAM, recommended 2GB+
- **Disk Space**: Minimum 100MB for installation, additional space for data

### Required Dependencies

- **C++17 Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake**: Version 3.16 or higher
- **OpenSSL**: Version 1.1.1 or higher
- **jsoncpp**: JSON parsing library (for JSON configuration support)
- **yaml-cpp**: YAML parsing library (optional, for YAML configuration support)
- **Platform Libraries**: See platform-specific sections below

## 🐧 Linux Installation

### Ubuntu/Debian (20.04+)

```bash
# Update package list
sudo apt-get update

# Install build dependencies
sudo apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libjsoncpp-dev \
    libyaml-cpp-dev \
    pkg-config \
    git

# Clone repository
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd

# Build and install
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### CentOS/RHEL 8+

```bash
# Enable EPEL repository
sudo dnf install -y epel-release

# Install build dependencies
sudo dnf install -y \
    gcc-c++ \
    cmake \
    openssl-devel \
    jsoncpp-devel \
    yaml-cpp-devel \
    pkgconfig \
    git

# Clone and build
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Fedora 35+

```bash
# Install build dependencies
sudo dnf install -y \
    gcc-c++ \
    cmake \
    openssl-devel \
    jsoncpp-devel \
    yaml-cpp-devel \
    pkgconfig \
    git

# Clone and build
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Arch Linux

```bash
# Install build dependencies
sudo pacman -S \
    base-devel \
    cmake \
    openssl \
    jsoncpp \
    yaml-cpp \
    pkg-config \
    git

# Clone and build
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

## 🍎 macOS Installation

### Homebrew (Recommended)

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install \
    cmake \
    openssl@3 \
    jsoncpp \
    yaml-cpp \
    pkg-config

# Clone repository
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd

# Build with Homebrew OpenSSL
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3) \
    -DOPENSSL_LIBRARIES=$(brew --prefix openssl@3)/lib

make -j$(sysctl -n hw.ncpu)
sudo make install

**Note**: If you encounter permission errors during installation on macOS, see the [Troubleshooting Guide](../troubleshooting/README.md#macos-installation-permission-issues) for solutions.
```

### Source Compilation

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install dependencies manually
brew install cmake jsoncpp yaml-cpp pkg-config

# Clone and build
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
sudo make install
```

## 🪟 Windows Installation

### Visual Studio 2017+ with CMake

```cmd
# Install Visual Studio 2017+ with C++ support
# Download from: https://visualstudio.microsoft.com/

# Install CMake
# Download from: https://cmake.org/download/

# Install vcpkg for dependencies
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install required packages
.\vcpkg install openssl jsoncpp yaml-cpp

# Clone repository
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd

# Build with CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# Install (run as Administrator)
cmake --install .
```

### Using vcpkg (Alternative)

```cmd
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install simple-rsyncd
.\vcpkg install simple-rsyncd
```

## 🐳 Docker Installation

### Quick Start

```bash
# Pull the official image
docker pull simple-rsyncd/simple-rsyncd:latest

# Run container
docker run -d \
    --name simple-rsyncd \
    -p 873:873 \
    -v /path/to/config:/etc/simple-rsyncd \
    -v /path/to/data:/var/lib/simple-rsyncd \
    simple-rsyncd/simple-rsyncd:latest
```

### Build from Source

```bash
# Clone repository
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd

# Build Docker image
docker build -t simple-rsyncd .

# Run container
docker run -d \
    --name simple-rsyncd \
    -p 873:873 \
    -v /path/to/config:/etc/simple-rsyncd \
    -v /path/to/data:/var/lib/simple-rsyncd \
    simple-rsyncd
```

### Docker Compose

```yaml
version: '3.8'
services:
  simple-rsyncd:
    image: simple-rsyncd/simple-rsyncd:latest
    ports:
      - "873:873"
    volumes:
      - ./config:/etc/simple-rsyncd
      - ./data:/var/lib/simple-rsyncd
    environment:
      - SIMPLE_RSYNCD_CONFIG=/etc/simple-rsyncd/rsyncd.conf
    restart: unless-stopped
```

## 📦 Package Manager Installation

### Ubuntu/Debian Package

```bash
# Add repository (when available)
# wget -qO - https://packages.simple-rsyncd.com/gpg.key | sudo apt-key add -
# echo "deb https://packages.simple-rsyncd.com/ubuntu focal main" | sudo tee /etc/apt/sources.list.d/simple-rsyncd.list

# Install package
sudo apt-get update
sudo apt-get install simple-rsyncd
```

### RPM Package (CentOS/RHEL/Fedora)

```bash
# Download RPM package
wget https://github.com/simple-rsyncd/simple-rsyncd/releases/latest/download/simple-rsyncd-0.1.0-1.x86_64.rpm

# Install
sudo rpm -i simple-rsyncd-0.1.0-1.x86_64.rpm
```

## 🔧 Post-Installation Setup

### Create Configuration Directory

```bash
# Create configuration directory
sudo mkdir -p /etc/simple-rsyncd
sudo mkdir -p /etc/simple-rsyncd/modules.d
sudo mkdir -p /etc/simple-rsyncd/ssl

# Create basic configuration
sudo tee /etc/simple-rsyncd/rsyncd.conf > /dev/null <<EOF
[global]
bind_address = 0.0.0.0
bind_port = 873
ssl_enabled = false
auth_enabled = false

[module:public]
path = /var/lib/simple-rsyncd/public
comment = Public files
read_only = true
list = true
EOF

# Create data directory
sudo mkdir -p /var/lib/simple-rsyncd/public
sudo chown -R rsync:rsync /var/lib/simple-rsyncd
```

### Create System User

```bash
# Create rsync user (if not exists)
sudo useradd -r -s /bin/false -d /var/lib/simple-rsyncd rsync

# Set permissions
sudo chown -R rsync:rsync /etc/simple-rsyncd
sudo chown -R rsync:rsync /var/lib/simple-rsyncd
```

### Systemd Service (Linux)

```bash
# Create systemd service file
sudo tee /etc/systemd/system/simple-rsyncd.service > /dev/null <<EOF
[Unit]
Description=Simple RSync Daemon
After=network.target

[Service]
Type=simple
User=rsync
Group=rsync
ExecStart=/usr/local/bin/simple-rsyncd start --config /etc/simple-rsyncd/rsyncd.conf
ExecReload=/bin/kill -HUP \$MAINPID
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable simple-rsyncd
sudo systemctl start simple-rsyncd
```

## ✅ Verification

### Check Installation

```bash
# Check binary
which simple-rsyncd
simple-rsyncd --version

# Check service status (Linux)
sudo systemctl status simple-rsyncd

# Test configuration
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf
```

### Test Connection

```bash
# List available modules
rsync rsync://localhost/

# Test file transfer
echo "test" > test.txt
rsync -avz test.txt rsync://localhost/public/
rsync -avz rsync://localhost/public/test.txt ./
```

## 🚨 Common Installation Issues

### Dependency Problems

**Problem**: `OpenSSL not found`
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel

# macOS
brew install openssl@3
```

**Problem**: `jsoncpp not found`
```bash
# Ubuntu/Debian
sudo apt-get install libjsoncpp-dev

# CentOS/RHEL
sudo yum install jsoncpp-devel

# macOS
brew install jsoncpp
```

### Compilation Errors

**Problem**: `C++17 not supported`
```bash
# Update compiler
# Ubuntu/Debian
sudo apt-get install g++-7

# Set compiler explicitly
export CC=gcc-7
export CXX=g++-7
```

**Problem**: `CMake version too old`
```bash
# Install newer CMake
# Ubuntu/Debian
sudo apt-get install cmake3

# Or build from source
wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0.tar.gz
tar -xzf cmake-3.24.0.tar.gz
cd cmake-3.24.0
./bootstrap && make && sudo make install
```

### Permission Issues

**Problem**: `Permission denied during install`
```bash
# Check user permissions
sudo -l

# Use sudo for installation
sudo make install
```

**Problem**: `Cannot create configuration directory`
```bash
# Create directories with proper permissions
sudo mkdir -p /etc/simple-rsyncd
sudo chown $USER:$USER /etc/simple-rsyncd
```

**Problem**: `Operation not permitted` on macOS
```bash
# Fix ownership of /usr/local directories
sudo chown -R $(whoami):admin /usr/local/include
sudo chown -R $(whoami):admin /usr/local/lib

# Or install to user directory
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install
```

## 📚 Next Steps

After successful installation:

1. **Configure the daemon**: See [Configuration Guide](../configuration/README.md)
2. **Learn basic usage**: See [User Guide](../user-guide/README.md)
3. **Set up SSL/TLS**: See [Security Configuration](../configuration/README.md#security)
4. **Create modules**: See [Module Configuration](../configuration/README.md#modules)

## 🆘 Getting Help

If you encounter installation issues:

1. **Check the troubleshooting guide** for common solutions
2. **Verify system requirements** and dependencies
3. **Check build logs** for specific error messages
4. **Create an issue** on GitHub with detailed information

---

**Installation complete?** Move on to the [User Guide](../user-guide/README.md) to learn how to use Simple RSync Daemon.
