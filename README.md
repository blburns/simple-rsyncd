# Simple RSync Daemon (simple-rsyncd)

A lightweight, secure, and feature-rich rsync daemon implementation in C++ designed for modern server environments.

## 🚀 Features

- **Multi-Platform Support**: Linux, macOS, and Windows
- **SSL/TLS Encryption**: Secure file transfers with modern cryptography
- **Authentication**: Multiple authentication methods (password, public key, OAuth2)
- **Access Control**: IP-based and network-based access restrictions
- **Module System**: Flexible module configuration with path-based access control
- **Configuration Overlays**: Support for configuration inheritance and hot-reloading
- **Rate Limiting**: Configurable connection and bandwidth limits
- **Monitoring**: Built-in metrics, health checks, and Prometheus integration
- **Logging**: Comprehensive logging with rotation and multiple outputs
- **Performance**: Optimized for high-throughput file transfers
- **Security**: Chroot support, privilege dropping, and path validation

## 📋 Requirements

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.16+**
- **OpenSSL 1.1.1+**
- **jsoncpp** (for JSON configuration parsing)
- **Linux**: glibc 2.17+, pthread, rt
- **macOS**: macOS 12.0+ (Monterey)
- **Windows**: Windows 10+ with Visual Studio 2017+

## 🛠️ Installation

### Quick Start

```bash
# Clone the repository
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd

# Build the project
mkdir build && cd build
cmake ..
make -j$(nproc)

# Install
sudo make install
```

### Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libjsoncpp-dev
```

#### CentOS/RHEL/Fedora
```bash
sudo yum install -y gcc-c++ cmake openssl-devel jsoncpp-devel
# or for newer versions:
sudo dnf install -y gcc-c++ cmake openssl-devel jsoncpp-devel
```

#### macOS
```bash
brew install openssl jsoncpp cmake

# If you encounter permission issues during installation, run:
./scripts/fix-macos-permissions.sh
```

#### Windows
- Install Visual Studio 2017+ with C++ support
- Install CMake and OpenSSL
- Use vcpkg for jsoncpp: `vcpkg install jsoncpp`

## 🚀 Usage

### Basic Commands

```bash
# Start the daemon
simple-rsyncd start --config /etc/simple-rsyncd/rsyncd.conf

# Start as daemon
simple-rsyncd start --daemon --config /etc/simple-rsyncd/rsyncd.conf

# Check status
simple-rsyncd status

# Test configuration
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

# Reload configuration
simple-rsyncd reload

# Stop daemon
simple-rsyncd stop

# Restart daemon
simple-rsyncd restart
```

### Configuration

The daemon uses a hierarchical configuration system with support for:

- **Global settings**: Network, SSL, authentication, logging
- **Module definitions**: Path-based access control and permissions
- **Configuration overlays**: Environment-specific customizations
- **Hot-reloading**: Configuration changes without restart

Example configuration:

```ini
[global]
bind_address = 0.0.0.0
bind_port = 873
ssl_enabled = true
auth_enabled = true

[module:backup]
path = /var/backup
comment = Backup storage
read_only = false
list = true
delete = true
overwrite = true

[module:public]
path = /var/public
comment = Public files
read_only = true
list = true
delete = false
overwrite = false
```

### Client Usage

```bash
# Basic rsync to daemon
rsync -avz /local/path/ rsync://server/module/

# With authentication
rsync -avz --password-file=/path/to/password /local/path/ rsync://user@server/module/

# With SSL/TLS
rsync -avz -e "rsync --rsh='openssl s_client -quiet -connect server:873'" /local/path/ /module/

# List available modules
rsync rsync://server/

# List module contents
rsync rsync://server/module/
```

## 🏗️ Architecture

### Core Components

- **RSyncDaemon**: Main server class managing connections and modules
- **Configuration**: Hierarchical configuration management with validation
- **Module**: Abstract interface for file system operations
- **RSyncSession**: Individual client connection handling
- **SSLContext**: SSL/TLS encryption management
- **Logger**: Comprehensive logging system

### Module System

Modules provide:
- **Path isolation**: Each module has its own root directory
- **Permission control**: Read-only, list, delete, and overwrite permissions
- **Pattern filtering**: Include/exclude patterns for files
- **Script hooks**: Pre/post transfer, delete, and list scripts
- **Environment variables**: Custom configuration per module

### Security Features

- **SSL/TLS encryption**: Secure file transfers
- **Authentication**: Multiple authentication methods
- **Access control**: IP and network restrictions
- **Path validation**: Prevention of directory traversal attacks
- **Privilege dropping**: Running with minimal permissions
- **Chroot support**: File system isolation

## 🔧 Configuration

### Configuration Files

- **Main config**: `/etc/simple-rsyncd/rsyncd.conf`
- **Module configs**: `/etc/simple-rsyncd/modules.d/`
- **User database**: `/etc/simple-rsyncd/users`
- **SSL certificates**: `/etc/simple-rsyncd/ssl/`

### Environment Variables

```bash
# Override configuration file location
export SIMPLE_RSYNCD_CONFIG=/path/to/config

# Enable debug mode
export SIMPLE_RSYNCD_DEBUG=1

# Set log level
export SIMPLE_RSYNCD_LOG_LEVEL=debug
```

### Configuration Inheritance

```ini
# Base configuration
[global]
ssl_enabled = true
auth_enabled = true

# Production override
[global:production]
ssl_enabled = true
auth_enabled = true
ssl_require_client_cert = true

# Development override
[global:development]
ssl_enabled = false
auth_enabled = false
```

## 📊 Monitoring

### Metrics

- **Connection statistics**: Active connections, total connections
- **Transfer statistics**: Bytes transferred, files transferred
- **Performance metrics**: Transfer rates, response times
- **Error rates**: Failed transfers, authentication failures

### Health Checks

- **Service health**: Daemon status and responsiveness
- **Module health**: Module availability and permissions
- **Resource usage**: Memory, CPU, and disk usage
- **SSL certificate**: Certificate expiration and validity

### Prometheus Integration

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'simple-rsyncd'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: '/metrics'
```

## 🔒 Security

### SSL/TLS Configuration

```ini
[global]
ssl_enabled = true
ssl_certificate_file = /etc/simple-rsyncd/ssl/server.crt
ssl_private_key_file = /etc/simple-rsyncd/ssl/server.key
ssl_ca_file = /etc/simple-rsyncd/ssl/ca.crt
ssl_cipher_suite = ECDHE-RSA-AES256-GCM-SHA384
ssl_tls_version = 1.2
```

### Authentication

```ini
[global]
auth_enabled = true
auth_method = password
auth_password_file = /etc/simple-rsyncd/users
auth_realm = simple-rsyncd
```

### Access Control

```ini
[global]
access_enabled = true
access_allowed_hosts = 127.0.0.1, 192.168.1.0/24
access_allowed_networks = 10.0.0.0/8
access_denied_hosts = 192.168.1.100
```

## 🚀 Performance

### Optimization Features

- **Connection pooling**: Efficient connection management
- **Buffer optimization**: Configurable buffer sizes
- **Compression**: Built-in compression support
- **Pipelining**: Parallel transfer operations
- **Checksum verification**: Fast file integrity checking

### Benchmarking

```bash
# Test transfer performance
rsync -avz --progress /large/directory/ rsync://server/module/

# Test concurrent connections
for i in {1..10}; do
  rsync -avz /test/dir/ rsync://server/module/ &
done
wait
```

## 🐳 Docker Support

### Quick Start

```bash
# Build image
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
    build: .
    ports:
      - "873:873"
    volumes:
      - ./config:/etc/simple-rsyncd
      - ./data:/var/lib/simple-rsyncd
    environment:
      - SIMPLE_RSYNCD_CONFIG=/etc/simple-rsyncd/rsyncd.conf
```

## 🔧 Development

### Building from Source

```bash
# Clone repository
git clone https://github.com/simple-rsyncd/simple-rsyncd.git
cd simple-rsyncd

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=ON

# Build
make -j$(nproc)

# Run tests
make test

# Install
sudo make install
```

### Development Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install -y \
  build-essential cmake \
  libssl-dev libjsoncpp-dev \
  clang-format cppcheck \
  valgrind gdb

# macOS
brew install \
  cmake openssl jsoncpp \
  clang-format cppcheck \
  valgrind lldb
```

### Code Style

- **C++17** standard
- **Clang-format** for code formatting
- **Cppcheck** for static analysis
- **Valgrind** for memory checking
- **Google Test** for unit testing

## 📚 Documentation

- **[Installation Guide](docs/installation/README.md)**: Platform-specific installation instructions
- **[User Guide](docs/user-guide/README.md)**: Complete usage documentation
- **[Configuration Guide](docs/configuration/README.md)**: Configuration reference
- **[Development Guide](docs/development/README.md)**: Contributing and development
- **[API Reference](docs/api/README.md)**: Developer API documentation
- **[Troubleshooting Guide](docs/troubleshooting/README.md)**: Common issues and solutions

**📖 [Start with the Installation Guide](docs/installation/README.md) for first-time setup**

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

### Development Setup

```bash
# Fork and clone
git clone https://github.com/your-username/simple-rsyncd.git
cd simple-rsyncd

# Create feature branch
git checkout -b feature/amazing-feature

# Make changes and test
make test

# Commit and push
git commit -m "Add amazing feature"
git push origin feature/amazing-feature

# Create pull request
```

## 📄 License

This project is licensed under the Apache License, Version 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **rsync** project for the protocol specification
- **OpenSSL** project for cryptographic libraries
- **jsoncpp** project for JSON parsing
- **CMake** project for build system
- **All contributors** who have helped improve this project

## 📞 Support

- **Documentation**: [docs/](docs/)
- **Issues**: [GitHub Issues](https://github.com/simple-rsyncd/simple-rsyncd/issues)
- **Discussions**: [GitHub Discussions](https://github.com/simple-rsyncd/simple-rsyncd/discussions)
- **Email**: SimpleDaemons

## 🔄 Changelog

See [CHANGELOG.md](CHANGELOG.md) for a complete list of changes.

---

**Simple RSync Daemon** - Secure, fast, and reliable file synchronization for modern servers.
