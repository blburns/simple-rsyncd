# Troubleshooting Guide

This guide helps you resolve common issues with Simple RSync Daemon.

**Version**: v0.3.0

**New Troubleshooting Topics in v0.3.0:**
- Password hashing and policy issues
- Session management problems
- Configuration hot-reload failures
- Environment variable substitution
- Enhanced logging and error handling

## 🚨 Quick Diagnosis

### Check Daemon Status

```bash
# Check if daemon is running
simple-rsyncd status

# Check process
ps aux | grep simple-rsyncd

# Check port usage
netstat -tlnp | grep :873
ss -tlnp | grep :873

# Check logs
tail -f /var/log/simple-rsyncd.log
```

### Common Symptoms

| Symptom | Possible Cause | Quick Check |
|---------|----------------|-------------|
| **Daemon won't start** | Configuration error, port in use, permissions | `simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf` |
| **Connection refused** | Daemon not running, wrong port, firewall | `simple-rsyncd status && netstat -tlnp \| grep :873` |
| **Authentication failed** | Wrong credentials, user not found | Check user database and authentication method |
| **SSL errors** | Certificate issues, wrong paths | Verify SSL certificate files and permissions |
| **Permission denied** | File/directory permissions, user rights | Check ownership and permissions |

## 🔧 Build Problems

### Compilation Errors

#### C++17 Not Supported

**Error**: `error: #error This file requires compiler and library support for the ISO C++ 2017 standard`

**Solution**:
```bash
# Check compiler version
g++ --version
clang++ --version

# Update compiler (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install g++-7 g++-8 g++-9

# Set compiler explicitly
export CC=gcc-7
export CXX=g++-7

# Rebuild
make clean && make
```

#### CMake Version Too Old

**Error**: `CMake 3.16 or higher is required. You are running version 3.10.2`

**Solution**:
```bash
# Install newer CMake (Ubuntu/Debian)
sudo apt-get install cmake3

# Or build from source
wget https://github.com/Kitware/CMake/releases/download/v3.24.0/cmake-3.24.0.tar.gz
tar -xzf cmake-3.24.0.tar.gz
cd cmake-3.24.0
./bootstrap && make && sudo make install

# Verify installation
cmake --version
```

#### OpenSSL Not Found

**Error**: `Could NOT find OpenSSL (missing: OpenSSL_LIBRARIES OpenSSL_INCLUDE_DIRS)`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL/Fedora
sudo yum install openssl-devel
sudo dnf install openssl-devel

# macOS
brew install openssl@3

# Windows
# Install via vcpkg: .\vcpkg install openssl
```

#### jsoncpp Not Found

**Error**: `Could NOT find jsoncpp`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install libjsoncpp-dev

# CentOS/RHEL/Fedora
sudo yum install jsoncpp-devel
sudo dnf install jsoncpp-devel

# macOS
brew install jsoncpp

# Windows
# Install via vcpkg: .\vcpkg install jsoncpp
```

### Linker Errors

#### Library Not Found

**Error**: `undefined reference to 'SSL_*'`

**Solution**:
```bash
# Check if OpenSSL is properly linked
pkg-config --libs openssl

# Rebuild with explicit paths
cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@3
make clean && make
```

#### Symbol Conflicts

**Error**: `multiple definition of 'main'`

**Solution**:
```bash
# Clean build directory
make clean
rm -rf build
mkdir build && cd build

# Reconfigure and rebuild
cmake ..
make
```

## 🚀 Runtime Issues

### Daemon Won't Start

#### Configuration Errors (v0.3.0 Enhanced)

**Problem**: `Configuration validation failed`

**Solution**:
```bash
# Test configuration
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

# Test JSON configuration (v0.3.0)
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.json

# Check configuration syntax
cat -n /etc/simple-rsyncd/rsyncd.conf

# Validate JSON syntax (v0.3.0)
cat /etc/simple-rsyncd/rsyncd.json | jq .

# Validate specific sections
simple-rsyncd validate --config /etc/simple-rsyncd/rsyncd.conf --module public

# Check environment variable substitution (v0.3.0)
# Ensure all referenced variables are set or have defaults
```

**Common Configuration Issues**:
- Missing `[global]` section
- Invalid port number (must be 1-65535)
- Invalid IP address format
- Missing required module configuration
- Invalid file paths
- **v0.3.0**: Environment variables not found (check `${VAR}` syntax)
- **v0.3.0**: Invalid JSON syntax (use `jq` to validate)
- **v0.3.0**: Configuration hot-reload failures (check reload logs)

#### Configuration Hot-Reload Issues (v0.3.0)

**Problem**: `Configuration reload failed` or hot-reload not working

**Diagnosis**:
```bash
# Check if hot-reload is enabled
grep auto_reload /etc/simple-rsyncd/rsyncd.conf

# Check reload interval
grep reload_interval /etc/simple-rsyncd/rsyncd.conf

# Check reload logs
grep -i "reload\|configuration" /var/log/simple-rsyncd.log

# Check file permissions
ls -la /etc/simple-rsyncd/rsyncd.conf
```

**Solutions**:
```bash
# Enable hot-reload
auto_reload = true
reload_interval = 30

# Ensure configuration file is readable
sudo chmod 644 /etc/simple-rsyncd/rsyncd.conf

# Check for configuration errors that prevent reload
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

# Manually trigger reload
simple-rsyncd reload

# Check reload status in logs
tail -f /var/log/simple-rsyncd.log | grep -i reload
```

#### Port Already in Use

**Problem**: `bind: address already in use`

**Solution**:
```bash
# Check what's using port 873
sudo lsof -i :873
sudo netstat -tlnp | grep :873

# Stop conflicting service
sudo systemctl stop rsync
sudo systemctl stop simple-rsyncd

# Or use different port
simple-rsyncd start --port 1873
```

#### Permission Issues

**Problem**: `Permission denied`

**Solution**:
```bash
# Check file permissions
ls -la /etc/simple-rsyncd/
ls -la /var/lib/simple-rsyncd/

# Fix ownership
sudo chown -R rsync:rsync /etc/simple-rsyncd
sudo chown -R rsync:rsync /var/lib/simple-rsyncd

# Fix permissions
sudo chmod 755 /etc/simple-rsyncd
sudo chmod 600 /etc/simple-rsyncd/rsyncd.conf
sudo chmod 755 /var/lib/simple-rsyncd
```

#### macOS Installation Permission Issues

**Problem**: `Operation not permitted` during `make install`

**Cause**: On macOS, some directories in `/usr/local/` may be owned by root, preventing normal users from installing files.

**Solution**:
```bash
# Option 1: Fix ownership (recommended)
sudo chown -R $(whoami):admin /usr/local/include
sudo chown -R $(whoami):admin /usr/local/lib

# Option 2: Install to user directory
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install

# Option 3: Use sudo for installation
sudo make install

# Option 4: Clean previous installation
sudo rm -rf /usr/local/include/simple_rsyncd
sudo rm -rf /usr/local/lib/libsimple-rsyncd*
sudo rm -f /usr/local/bin/simple-rsyncd*
make install
```

**Prevention**: After fixing ownership, you can prevent this issue by:
```bash
# Set proper ownership for future installations
sudo chown -R $(whoami):admin /usr/local
sudo chmod 755 /usr/local
```

### Connection Problems

#### Connection Refused

**Problem**: `Connection refused`

**Diagnosis**:
```bash
# Check daemon status
simple-rsyncd status

# Check if daemon is listening
netstat -tlnp | grep simple-rsyncd

# Check firewall
sudo ufw status
sudo iptables -L

# Test local connection
telnet localhost 873
```

**Solutions**:
```bash
# Start daemon if not running
simple-rsyncd start

# Check bind address
grep bind_address /etc/simple-rsyncd/rsyncd.conf

# Allow port in firewall
sudo ufw allow 873
sudo iptables -A INPUT -p tcp --dport 873 -j ACCEPT
```

#### SSL/TLS Issues

**Problem**: `SSL handshake failed`

**Diagnosis**:
```bash
# Check SSL configuration
grep -i ssl /etc/simple-rsyncd/rsyncd.conf

# Verify certificate files
ls -la /etc/simple-rsyncd/ssl/
openssl x509 -in /etc/simple-rsyncd/ssl/server.crt -text -noout

# Check certificate expiration
openssl x509 -in /etc/simple-rsyncd/ssl/server.crt -noout -dates
```

**Solutions**:
```bash
# Regenerate certificates if expired
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

# Fix file permissions
sudo chmod 600 /etc/simple-rsyncd/ssl/server.key
sudo chmod 644 /etc/simple-rsyncd/ssl/server.crt

# Restart daemon
simple-rsyncd restart
```

### Authentication Issues

#### Password Authentication Failed (v0.3.0 Enhanced)

**Problem**: `Authentication failed`

**Diagnosis**:
```bash
# Check authentication method
grep auth_method /etc/simple-rsyncd/rsyncd.conf

# Check user database
cat /etc/simple-rsyncd/users

# Check if passwords are hashed (v0.3.0)
grep sha256 /etc/simple-rsyncd/users

# Check password policies (v0.3.0)
grep password_policy /etc/simple-rsyncd/rsyncd.conf

# Check for account lockout (v0.3.0)
grep -i "account locked\|password expired" /var/log/simple-rsyncd.log

# Check file permissions
ls -la /etc/simple-rsyncd/users
```

**Solutions**:
```bash
# Create user database
sudo touch /etc/simple-rsyncd/users
sudo chmod 600 /etc/simple-rsyncd/users
sudo chown rsync:rsync /etc/simple-rsyncd/users

# Add user with plain text password (will be auto-hashed)
echo "username:password" | sudo tee -a /etc/simple-rsyncd/users

# Or add user with pre-hashed password (v0.3.0)
# Format: username:sha256:salt:hash
echo "username:sha256:abc123...:def456..." | sudo tee -a /etc/simple-rsyncd/users

# Check password policy requirements (v0.3.0)
# Ensure password meets minimum length and complexity requirements

# Test authentication
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf
```

#### Password Policy Issues (v0.3.0)

**Problem**: `Password does not meet policy requirements`

**Diagnosis**:
```bash
# Check password policy settings
grep password_policy /etc/simple-rsyncd/rsyncd.conf

# Check policy requirements
# - Minimum length
# - Uppercase/lowercase requirements
# - Digit requirements
# - Special character requirements
```

**Solutions**:
```bash
# Adjust password policy or use stronger password
# Update configuration:
password_policy_min_length = 8
password_policy_require_uppercase = true
password_policy_require_lowercase = true
password_policy_require_digits = true

# Reload configuration
simple-rsyncd reload
```

#### Session Issues (v0.3.0)

**Problem**: `Session expired` or session-related errors

**Diagnosis**:
```bash
# Check session timeout settings
grep session_timeout /etc/simple-rsyncd/rsyncd.conf

# Check session management status
grep enable_session_management /etc/simple-rsyncd/rsyncd.conf

# View session-related logs
grep -i session /var/log/simple-rsyncd.log
```

**Solutions**:
```bash
# Increase session timeout
session_timeout = 7200  # 2 hours

# Or disable session management for testing
enable_session_management = false

# Reload configuration
simple-rsyncd reload
```

#### OAuth2 Issues

**Problem**: `OAuth2 authentication failed`

**Diagnosis**:
```bash
# Check OAuth2 configuration
grep -i oauth /etc/simple-rsyncd/rsyncd.conf

# Check environment variables
env | grep -i oauth
```

**Solutions**:
```bash
# Verify OAuth2 credentials
# Check client ID, secret, and issuer URL

# Test OAuth2 connection
curl -I https://accounts.google.com/.well-known/openid_configuration

# Check token validation
# Use OAuth2 debugging tools
```

## 📁 Module Issues

### Module Not Found

**Problem**: `Module 'backup' not found`

**Diagnosis**:
```bash
# Check module configuration
grep -A 10 "\[module:backup\]" /etc/simple-rsyncd/rsyncd.conf

# List available modules
simple-rsyncd status --verbose

# Check module files
ls -la /etc/simple-rsyncd/modules.d/
```

**Solutions**:
```bash
# Add missing module
cat >> /etc/simple-rsyncd/rsyncd.conf <<EOF

[module:backup]
path = /var/backup
comment = Backup storage
read_only = false
list = true
delete = true
EOF

# Create module directory
sudo mkdir -p /var/backup
sudo chown rsync:rsync /var/backup

# Reload configuration
simple-rsyncd reload
```

### Module Permission Issues

**Problem**: `Permission denied for module operation`

**Diagnosis**:
```bash
# Check module permissions
grep -A 15 "\[module:backup\]" /etc/simple-rsyncd/rsyncd.conf

# Check file system permissions
ls -la /var/backup/
```

**Solutions**:
```bash
# Fix module permissions
sudo chown -R rsync:rsync /var/backup
sudo chmod 755 /var/backup

# Update module configuration
# Set appropriate permissions in rsyncd.conf
```

## 🔒 Security Issues

### SSL Certificate Problems

#### Self-Signed Certificate Warnings

**Problem**: `SSL certificate verification failed`

**Solution**:
```bash
# Generate proper CA-signed certificate
# Or add self-signed certificate to client trust store

# For testing, skip verification (not recommended for production)
rsync -avz -e "rsync --rsh='openssl s_client -quiet -connect server:873'" /local/path/ /module/
```

#### Certificate Chain Issues

**Problem**: `unable to get local issuer certificate`

**Solution**:
```bash
# Include full certificate chain
cat server.crt ca.crt > server-chain.crt

# Update configuration
ssl_certificate_file = /etc/simple-rsyncd/ssl/server-chain.crt
```

### Access Control Issues

#### IP Restrictions

**Problem**: `Access denied from IP address`

**Diagnosis**:
```bash
# Check access control configuration
grep -i access /etc/simple-rsyncd/rsyncd.conf

# Check client IP
# Use logging to see client IP addresses
```

**Solutions**:
```bash
# Add client IP to allowed hosts
access_allowed_hosts = 192.168.1.100, 10.0.0.0/8

# Or disable access control for testing
access_enabled = false

# Reload configuration
simple-rsyncd reload
```

## 📊 Performance Issues

### Slow Transfer Speeds

**Problem**: Transfer rates are much lower than expected

**Diagnosis**:
```bash
# Check system resources
htop
iostat
netstat -i

# Check daemon configuration
grep -i "buffer\|compression\|threads" /etc/simple-rsyncd/rsyncd.conf
```

**Solutions**:
```bash
# Optimize buffer size
buffer_size = 32768

# Enable compression
compression_enabled = true
compression_level = 6

# Increase worker threads
worker_threads = 8

# Check network configuration
# Verify MTU settings, network card configuration
```

### High Memory Usage

**Problem**: Daemon using excessive memory

**Diagnosis**:
```bash
# Check memory usage
ps aux | grep simple-rsyncd
free -h

# Check for memory leaks
valgrind --tool=memcheck --leak-check=full ./bin/simple-rsyncd-x86_64
```

**Solutions**:
```bash
# Limit memory usage
max_memory = 512
max_memory_per_connection = 32

# Reduce buffer sizes
buffer_size = 8192

# Check for memory leaks in code
# Use memory profiling tools
```

## 🐳 Docker Issues

### Container Won't Start

**Problem**: Docker container exits immediately

**Diagnosis**:
```bash
# Check container logs
docker logs simple-rsyncd

# Check container status
docker ps -a

# Check configuration volume
docker exec -it simple-rsyncd ls -la /etc/simple-rsyncd/
```

**Solutions**:
```bash
# Fix configuration
# Ensure rsyncd.conf exists and is valid

# Check file permissions
docker exec -it simple-rsyncd chown -R rsync:rsync /etc/simple-rsyncd

# Restart container
docker restart simple-rsyncd
```

### Volume Mount Issues

**Problem**: Configuration or data not accessible

**Solution**:
```bash
# Check volume mounts
docker inspect simple-rsyncd | grep -A 10 Mounts

# Fix volume permissions
sudo chown -R 1000:1000 /path/to/config
sudo chown -R 1000:1000 /path/to/data

# Restart container
docker restart simple-rsyncd
```

## 🔍 Debugging

### Enable Debug Logging (v0.3.0 Enhanced)

```bash
# Set debug level
export SIMPLE_RSYNCD_LOG_LEVEL=debug

# Or update configuration
log_level = debug

# Configure log format (v0.3.0)
log_format = json  # or text

# Configure log rotation (v0.3.0)
log_max_size = 10MB
log_max_files = 5

# Restart daemon
simple-rsyncd restart

# Watch logs
tail -f /var/log/simple-rsyncd.log

# View JSON logs with jq (v0.3.0)
tail -f /var/log/simple-rsyncd.log | jq .

# Filter logs by level (v0.3.0)
grep '"level":"ERROR"' /var/log/simple-rsyncd.log | jq .
```

### Error Handling (v0.3.0)

The daemon now provides comprehensive error codes and context:

```bash
# View errors with error codes
grep ERROR /var/log/simple-rsyncd.log

# View errors by category
grep "\[AUTHENTICATION\]" /var/log/simple-rsyncd.log
grep "\[CONFIGURATION\]" /var/log/simple-rsyncd.log

# View error recovery suggestions
grep -A 5 "recovery_suggestion" /var/log/simple-rsyncd.log

# Parse JSON error logs
cat /var/log/simple-rsyncd.log | jq 'select(.level == "ERROR")'
```

For detailed error handling information, see the [Error Handling Guide](../error-handling.md).

### Verbose Output

```bash
# Test with verbose output
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf --verbose

# Start with debug mode
simple-rsyncd start --debug

# Check status with details
simple-rsyncd status --verbose
```

### Network Debugging

```bash
# Test network connectivity
telnet localhost 873
nc -zv localhost 873

# Check SSL connection
openssl s_client -connect localhost:873

# Use tcpdump for packet analysis
sudo tcpdump -i lo -n port 873
```

## 📋 Common Error Messages

### Configuration Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Invalid configuration file` | Syntax error in rsyncd.conf | Check configuration syntax |
| `Module path not found` | Module directory doesn't exist | Create directory and set permissions |
| `SSL certificate not found` | Certificate file missing or inaccessible | Check file paths and permissions |
| `Invalid port number` | Port outside valid range (1-65535) | Use valid port number |
| `Bind address invalid` | Invalid IP address format | Use valid IP address (0.0.0.0, 127.0.0.1, etc.) |

### Network Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Address already in use` | Port 873 occupied by another service | Stop conflicting service or use different port |
| `Permission denied` | Insufficient privileges to bind port | Run as root or use port > 1024 |
| `Connection refused` | Daemon not running or wrong port | Start daemon or check port configuration |
| `SSL handshake failed` | Certificate issues or SSL misconfiguration | Check SSL certificates and configuration |

### Authentication Errors (v0.3.0 Enhanced)

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Authentication failed` | Wrong credentials or user not found | Check user database and credentials |
| `User not found` | User doesn't exist in database | Add user to authentication database |
| `Insufficient permissions` | User lacks required permissions | Check module permissions and user access |
| `Authentication method not supported` | Unsupported auth method configured | Use supported authentication method |
| `Password does not meet policy requirements` | Password too weak (v0.3.0) | Use stronger password or adjust policy |
| `Account locked` | Too many failed login attempts (v0.3.0) | Wait for lockout period or reset account |
| `Password expired` | Password has expired (v0.3.0) | Change password |
| `Session expired` | Session timeout exceeded (v0.3.0) | Re-authenticate or increase timeout |

### Configuration Errors (v0.3.0 Enhanced)

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Invalid configuration file` | Syntax error in rsyncd.conf or rsyncd.json | Check configuration syntax |
| `Environment variable not found` | Missing environment variable (v0.3.0) | Set variable or provide default value |
| `Configuration reload failed` | Invalid configuration during hot-reload (v0.3.0) | Fix configuration errors and try again |
| `JSON parsing error` | Invalid JSON syntax (v0.3.0) | Validate JSON syntax |

## 🆘 Getting Help

### Before Asking for Help

1. **Check this guide** for your specific error
2. **Enable debug logging** and check logs
3. **Test configuration** with validation commands
4. **Verify system requirements** and dependencies
5. **Check recent changes** that might have caused the issue

### Information to Provide

When asking for help, include:

- **Error message** (exact text)
- **System information** (OS, version, architecture)
- **Simple RSync Daemon version**
- **Configuration file** (relevant sections)
- **Log output** (with debug logging enabled)
- **Steps to reproduce** the issue
- **What you've already tried**

### Resources

- **GitHub Issues**: [Create an issue](https://github.com/simple-rsyncd/simple-rsyncd/issues)
- **Documentation**: Check other sections of this guide
- **Source Code**: Examine the implementation for clues
- **Community**: Join discussions and ask questions

## 📚 Next Steps

After resolving your issue:

1. **Document the solution** for future reference
2. **Check for similar issues** in the codebase
3. **Consider contributing** a fix or documentation
4. **Monitor logs** to ensure the issue doesn't recur
5. **Implement preventive measures** to avoid similar issues

---

**Still having trouble?** Check the [User Guide](../user-guide/README.md) for usage patterns or the [Configuration Guide](../configuration/README.md) for setup details.
