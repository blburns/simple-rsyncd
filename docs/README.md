# Simple RSync Daemon Documentation

Welcome to the comprehensive documentation for Simple RSync Daemon (simple-rsyncd), a lightweight, secure, and feature-rich rsync daemon implementation in C++.

## 📚 Documentation Sections

### 🚀 [Installation Guide](installation/README.md)
Complete installation instructions for all supported platforms:
- **Linux**: Ubuntu/Debian, CentOS/RHEL/Fedora
- **macOS**: Homebrew and source compilation
- **Windows**: Visual Studio and vcpkg
- **Docker**: Container deployment
- **Dependencies**: Required libraries and tools

### 🔧 [Installation Issues Resolved](INSTALLATION_ISSUES_RESOLVED.md)
Summary of common installation problems and their solutions:
- **macOS permission issues** and fixes
- **Permission fix script** for automatic resolution
- **Alternative installation methods**
- **Prevention strategies**

### 👥 [User Guide](user-guide/README.md)
Comprehensive usage documentation:
- **Quick Start**: Get up and running in minutes
- **Basic Commands**: Start, stop, status, and configuration
- **Client Usage**: How to connect from rsync clients
- **Module Management**: Creating and configuring modules
- **Security**: SSL/TLS, authentication, and access control

### ⚙️ [Configuration Guide](configuration/README.md)
Detailed configuration reference:
- **Configuration Files**: Main config and module configs
- **Global Settings**: Network, SSL, authentication, logging
- **Module Configuration**: Paths, permissions, and access control
- **Environment Variables**: Runtime configuration overrides
- **Configuration Examples**: Production and development setups

### 🔧 [Development Guide](development/README.md)
Developer documentation:
- **Building from Source**: Compilation and development setup
- **Code Structure**: Architecture and component overview
- **Testing**: Unit tests and integration testing
- **Contributing**: Guidelines for contributors
- **Code Style**: Standards and best practices

### 📖 [API Reference](api/README.md)
Technical API documentation:
- **Core Classes**: RSyncDaemon, Configuration, Module
- **Interfaces**: Abstract classes and virtual methods
- **Error Handling**: Exception types and error codes
- **Examples**: Code samples and usage patterns

### 🐛 [Troubleshooting Guide](troubleshooting/README.md)
Common issues and solutions:
- **Build Problems**: Compilation errors and solutions
- **Runtime Issues**: Daemon startup and operation problems
- **Performance**: Optimization and tuning
- **Security**: SSL certificate and authentication issues

### ⚠️ [Error Handling Guide](error-handling.md) (v0.3.0)
Comprehensive error handling documentation:
- **Error Categories**: Organized error types by domain
- **Error Codes**: Numeric codes for programmatic handling
- **Error Context**: Rich contextual information
- **Error Recovery**: Suggestions for resolving errors
- **Structured Error Reporting**: JSON and text formats

## 🎯 Quick Navigation

| Task | Documentation |
|------|---------------|
| **First Time Setup** | [Installation Guide](installation/README.md) |
| **Basic Usage** | [User Guide](user-guide/README.md) |
| **Configuration** | [Configuration Guide](configuration/README.md) |
| **Error Handling** | [Error Handling Guide](error-handling.md) |
| **Development** | [Development Guide](development/README.md) |
| **API Reference** | [API Reference](api/README.md) |
| **Problem Solving** | [Troubleshooting Guide](troubleshooting/README.md) |

## 🔍 Search Documentation

Use the table of contents in each section or search for specific topics:

- **Commands**: `simple-rsyncd start`, `simple-rsyncd stop`, etc.
- **Configuration**: SSL settings, authentication, modules
- **Platforms**: Linux, macOS, Windows, Docker
- **Features**: Security, performance, monitoring

## 📖 Additional Resources

- **[Main README](../README.md)**: Project overview and quick start
- **[LICENSE](../LICENSE)**: Apache 2.0 license
- **[CHANGELOG](../CHANGELOG.md)**: Version history and changes
- **[GitHub Issues](https://github.com/simple-rsyncd/simple-rsyncd/issues)**: Bug reports and feature requests

## 🤝 Getting Help

If you can't find what you're looking for:

1. **Check the troubleshooting guide** for common issues
2. **Search existing issues** on GitHub
3. **Create a new issue** with detailed information
4. **Join discussions** in GitHub Discussions

## 📝 Documentation Updates

This documentation is maintained alongside the codebase. If you find errors or areas for improvement:

1. **Fork the repository**
2. **Make your changes** to the docs
3. **Submit a pull request**
4. **Help improve the documentation** for everyone

---

**Need help?** Start with the [Installation Guide](installation/README.md) for first-time setup, or jump to the [User Guide](user-guide/README.md) for usage instructions.
