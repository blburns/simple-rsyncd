# Simple RSync Daemon (simple-rsyncd)

A lightweight rsync-style daemon in C++ for modern server environments.

> **Release status: v0.4.0 Security-ready beta** — TLS, IP/CIDR access control, rate limiting, privilege drop, and symlink hardening are implemented (`ctest` 7/7). Still **not** a native `rsyncd` drop-in or general production release. See [project/PRODUCTION_GATE.md](project/PRODUCTION_GATE.md) and [docs/security/THREAT_MODEL.md](docs/security/THREAT_MODEL.md).

## 🚀 Features

### Implemented (v0.4.0)

- **Multi-platform packaging**: Linux (DEB/RPM), FreeBSD (PKG), macOS (PKG/DMG)
- **Module system**: Path isolation, permissions, include/exclude, symlink escape blocking
- **Custom protocol**: LIST / GET / PUT / DELETE / STAT with `key=value` arguments
- **TLS 1.2+**: Optional encryption via `[ssl]` config (`SSLContext` + handshake on accept)
- **Access control**: IP/CIDR allow/deny enforced at connection accept
- **Rate limiting**: Per-IP connection limits (minute/hour windows)
- **Hardening**: Privilege drop after bind; optional chroot (documented)
- **Authentication**: SHA-256 password hashes; `reject_plaintext_passwords` option
- **Tests**: 7 CTest suites including security and TLS handshake tests
- **CI**: Build, test, and cppcheck on push

### Planned (Gate 3 / production)

- **Native rsync client compatibility** — client matrix (v0.5.0)
- **bcrypt/argon2** password storage upgrade
- **Public-key auth** — parsing exists; verification incomplete
- **OAuth2, Prometheus** — not implemented
- **YAML hot-reload** — partial; prefer INI

## 📋 Requirements

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.16+**
- **OpenSSL 1.1.1+**
- **jsoncpp** (for JSON configuration parsing)
- **yaml-cpp** (optional, for YAML configuration parsing)
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
sudo apt-get install -y build-essential cmake libssl-dev libjsoncpp-dev libyaml-cpp-dev
```

#### CentOS/RHEL/Fedora
```bash
sudo yum install -y gcc-c++ cmake openssl-devel jsoncpp-devel yaml-cpp-devel
# or for newer versions:
sudo dnf install -y gcc-c++ cmake openssl-devel jsoncpp-devel yaml-cpp-devel
```

#### macOS
```bash
brew install openssl jsoncpp yaml-cpp cmake

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
- **Configuration formats**: INI, JSON, and YAML (v0.3.0)
- **Environment variables**: Variable substitution in config files (v0.3.0)
- **Hot-reloading**: Automatic configuration reload on file changes (v0.3.0)
- **Configuration validation**: Comprehensive validation with detailed errors

Example configurations:

**INI Format:**
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
```

**YAML Format:**
```yaml
global:
  bind_address: 0.0.0.0
  bind_port: 873
  ssl:
    enabled: true
  auth:
    enabled: true

modules:
  backup:
    path: /var/backup
    comment: Backup storage
    read_only: false
    list: true
    allow_delete: true
    overwrite: true
```

**JSON Format:**
```json
{
  "global": {
    "bind_address": "0.0.0.0",
    "bind_port": 873
  },
  "ssl": {
    "enabled": true
  },
  "auth": {
    "enabled": true
  },
  "modules": {
    "backup": {
      "path": "/var/backup",
      "comment": "Backup storage",
      "read_only": false,
      "list": true,
      "allow_delete": true,
      "overwrite": true
    }
  }
}
```

### Client Usage

simple-rsyncd uses a **simplified custom protocol** (not full native rsync wire format). For beta testing, use the daemon’s protocol commands or the project test client.

```bash
# Example protocol commands (over TCP to bind_port, default 873)
# LIST <module> <path> [key=value ...]
# GET  <module> <path>
# PUT  <module> <path>
# DELETE <module> <path> [recursive=true]

# List available modules (after connecting and authenticating if enabled)
# See docs/ and src/tests/ for worked examples.
```

Native `rsync rsync://…` compatibility is **not guaranteed** in v0.3.1; see the v0.5.0 client matrix in [project/PRODUCTION_GATE.md](project/PRODUCTION_GATE.md).

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

### Authentication (v0.3.0 Enhanced)

```ini
[global]
auth_enabled = true
auth_method = password
auth_password_file = /etc/simple-rsyncd/users
auth_realm = simple-rsyncd

# Password policies (v0.3.0)
password_policy_min_length = 8
password_policy_require_uppercase = true
password_policy_require_lowercase = true
password_policy_require_digits = true
password_policy_expiration_hours = 2160  # 90 days

# Session management (v0.3.0)
session_timeout = 3600
enable_session_management = true
```

**Password Security:**
- Passwords are automatically hashed using SHA-256 with salt
- Supports both plain text (backward compatible) and pre-hashed passwords
- Password policies enforce complexity requirements
- Password expiration and account lockout support
- User database with CRUD operations

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
- **[Configuration Guide](docs/configuration/README.md)**: Configuration reference (v0.3.0: JSON format, env vars, hot-reload)
- **[Error Handling Guide](docs/error-handling.md)**: Comprehensive error handling system (v0.3.0)
- **[Development Guide](docs/development/README.md)**: Contributing and development
- **[API Reference](docs/api/README.md)**: Developer API documentation
- **[Troubleshooting Guide](docs/troubleshooting/README.md)**: Common issues and solutions

**📖 [Start with the Installation Guide](docs/installation/README.md) for first-time setup**

**🆕 v0.3.0 Features:**
- Password hashing and policies
- User database and session management
- Configuration hot-reload
- Environment variable substitution
- Enhanced logging with rotation
- Comprehensive error handling

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
