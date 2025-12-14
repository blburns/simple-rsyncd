# Configuration Guide

This guide covers all configuration options for Simple RSync Daemon, from basic setup to advanced features.

**Version**: v0.3.0

**New in v0.3.0:**
- Environment variable substitution in configuration files
- Automatic configuration hot-reload
- JSON configuration format support
- Password hashing and policies
- User database management
- Session management
- Enhanced logging with rotation
- Structured JSON logging

## 📁 Configuration Files

### File Locations

| Platform | Configuration Directory | Default Config |
|----------|------------------------|----------------|
| **Linux** | `/etc/simple-rsyncd/` | `rsyncd.conf` |
| **macOS** | `/usr/local/etc/simple-rsyncd/` | `rsyncd.conf` |
| **Windows** | `C:\Program Files\simple-rsyncd\config\` | `rsyncd.conf` |
| **Docker** | `/etc/simple-rsyncd/` | `rsyncd.conf` |

### Configuration Structure

```
/etc/simple-rsyncd/
├── rsyncd.conf              # Main configuration file (INI format)
├── rsyncd.json              # Alternative JSON configuration (v0.3.0)
├── modules.d/               # Module configuration files
│   ├── 01-public.conf      # Public module
│   ├── 02-backup.conf      # Backup module
│   └── 03-private.conf     # Private module
├── ssl/                     # SSL certificates
│   ├── server.crt          # Server certificate
│   ├── server.key          # Server private key
│   └── ca.crt              # CA certificate
├── users                    # Password file (plain text or hashed)
├── userdb                   # User database (v0.3.0)
└── templates/               # Configuration templates
```

### Configuration File Formats (v0.3.0)

The daemon supports two configuration formats:

1. **INI Format** (traditional): `.conf` files
2. **JSON Format** (v0.3.0): `.json` files

The format is automatically detected by file extension. Both formats support the same configuration options.

## ⚙️ Global Configuration

### Basic Settings

```ini
[global]
# Network settings
bind_address = 0.0.0.0
bind_port = 873
max_connections = 100
connection_timeout = 300

# SSL/TLS settings
ssl_enabled = true
ssl_certificate_file = /etc/simple-rsyncd/ssl/server.crt
ssl_private_key_file = /etc/simple-rsyncd/ssl/server.key

# Authentication
auth_enabled = true
auth_method = password
auth_realm = simple-rsyncd
auth_password_file = /etc/simple-rsyncd/users

# Configuration hot-reload (v0.3.0)
auto_reload = true
reload_interval = 30

# Logging
log_level = info
log_file = /var/log/simple-rsyncd.log
log_format = json
log_max_size = 10MB
log_max_files = 5
log_compress_old_logs = true

# Performance
max_transfer_rate = 0
buffer_size = 8192
```

### Network Configuration

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `bind_address` | IP address to bind to | `0.0.0.0` | `127.0.0.1` |
| `bind_port` | Port to listen on | `873` | `1873` |
| `max_connections` | Maximum concurrent connections | `100` | `500` |
| `connection_timeout` | Connection timeout in seconds | `300` | `600` |
| `keepalive` | Enable TCP keepalive | `true` | `false` |
| `tcp_nodelay` | Enable TCP_NODELAY | `true` | `false` |

### SSL/TLS Configuration

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `ssl_enabled` | Enable SSL/TLS | `false` | `true` |
| `ssl_certificate_file` | Server certificate path | - | `/etc/ssl/certs/server.crt` |
| `ssl_private_key_file` | Private key path | - | `/etc/ssl/private/server.key` |
| `ssl_ca_file` | CA certificate path | - | `/etc/ssl/certs/ca.crt` |
| `ssl_cipher_suite` | Cipher suite | `HIGH` | `ECDHE-RSA-AES256-GCM-SHA384` |
| `ssl_tls_version` | Minimum TLS version | `1.2` | `1.3` |
| `ssl_require_client_cert` | Require client certificates | `false` | `true` |

### Authentication Configuration

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `auth_enabled` | Enable authentication | `false` | `true` |
| `auth_method` | Authentication method | `password` | `oauth2` |
| `auth_password_file` | Password file path | - | `/etc/simple-rsyncd/users` |
| `auth_realm` | Authentication realm | `simple-rsyncd` | `My Server` |
| `auth_oauth2_client_id` | OAuth2 client ID | - | `your_client_id` |
| `auth_oauth2_client_secret` | OAuth2 client secret | - | `your_secret` |
| `auth_oauth2_issuer` | OAuth2 issuer URL | - | `https://accounts.google.com` |
| `session_timeout` | Session timeout in seconds | `3600` | `7200` |
| `enable_session_management` | Enable session tracking | `true` | `false` |

### Password Policies (v0.3.0)

Password policies can be configured to enforce security requirements:

```ini
[global]
# Password policy settings
password_policy_min_length = 8
password_policy_require_uppercase = true
password_policy_require_lowercase = true
password_policy_require_digits = true
password_policy_require_special = false
password_policy_expiration_hours = 2160  # 90 days
password_policy_max_failed_attempts = 5
password_policy_lockout_duration = 30  # minutes
```

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `password_policy_min_length` | Minimum password length | `8` | `12` |
| `password_policy_require_uppercase` | Require uppercase letters | `false` | `true` |
| `password_policy_require_lowercase` | Require lowercase letters | `false` | `true` |
| `password_policy_require_digits` | Require digits | `false` | `true` |
| `password_policy_require_special` | Require special characters | `false` | `true` |
| `password_policy_expiration_hours` | Password expiration (0 = no expiration) | `0` | `2160` (90 days) |
| `password_policy_max_failed_attempts` | Max failed login attempts before lockout | `5` | `10` |
| `password_policy_lockout_duration` | Account lockout duration in minutes | `30` | `60` |

### Logging Configuration (v0.3.0 Enhanced)

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `log_level` | Log level | `info` | `debug` |
| `log_file` | Log file path | - | `/var/log/simple-rsyncd.log` |
| `log_format` | Log format (`text` or `json`) | `text` | `json` |
| `log_max_size` | Max log file size before rotation | `10MB` | `100MB` |
| `log_max_files` | Max rotated log files to keep | `5` | `10` |
| `log_compress_old_logs` | Compress old log files | `true` | `false` |
| `log_output` | Log destinations | `console` | `file,console` |

**Structured Logging (JSON Format):**
```json
{
  "timestamp": "2024-12-14 10:30:45",
  "level": "INFO",
  "message": "Configuration reloaded successfully",
  "component": "Configuration",
  "fields": {
    "config_file": "/etc/simple-rsyncd/rsyncd.conf",
    "modules_reloaded": "3"
  }
}
```

**Log Rotation:**
- Logs are automatically rotated when they reach `log_max_size`
- Old logs are renamed with numeric suffixes (`.1`, `.2`, etc.)
- When `log_max_files` is reached, oldest logs are compressed or deleted
- Compression creates `.gz` files for old logs

### Performance Configuration

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `max_transfer_rate` | Max transfer rate (KB/s) | `0` (unlimited) | `1024` |
| `buffer_size` | Transfer buffer size | `8192` | `16384` |
| `compression_level` | Compression level | `6` | `9` |
| `max_memory` | Max memory usage (MB) | `512` | `1024` |
| `worker_threads` | Number of worker threads | `4` | `8` |

## 📁 Module Configuration

### Basic Module

```ini
[module:public]
# Basic settings
path = /var/lib/simple-rsyncd/public
comment = Public files
read_only = true

# Permissions
list = true
delete = false
overwrite = false
create = false

# Access control
allowed_users = *
allowed_hosts = *
```

### Advanced Module

```ini
[module:backup]
# Basic settings
path = /var/backup
comment = Backup storage
read_only = false

# Permissions
list = true
delete = true
overwrite = true
create = true

# Access control
allowed_users = admin,backup
allowed_hosts = 192.168.1.0/24,10.0.0.0/8

# Patterns
include = *.tar.gz
include = *.sql
include = */backup/*
exclude = *.tmp
exclude = */temp/*

# Scripts
pre_transfer_script = /usr/local/bin/pre-backup.sh
post_transfer_script = /usr/local/bin/post-backup.sh
delete_script = /usr/local/bin/pre-delete.sh

# Environment
env[BACKUP_DATE] = $(date +%Y%m%d)
env[BACKUP_USER] = $USER
```

### Module Options Reference

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `path` | Module root directory | - | `/var/data` |
| `comment` | Module description | - | `Data storage` |
| `read_only` | Read-only module | `false` | `true` |
| `list` | Allow directory listing | `true` | `false` |
| `delete` | Allow file deletion | `false` | `true` |
| `overwrite` | Allow file overwriting | `false` | `true` |
| `create` | Allow file creation | `true` | `false` |
| `allowed_users` | Allowed users | `*` | `admin,user1` |
| `allowed_hosts` | Allowed hosts | `*` | `192.168.1.0/24` |
| `include` | Include patterns | - | `*.txt` |
| `exclude` | Exclude patterns | - | `*.tmp` |
| `pre_transfer_script` | Pre-transfer script | - | `/path/to/script.sh` |
| `post_transfer_script` | Post-transfer script | - | `/path/to/script.sh` |
| `delete_script` | Pre-delete script | - | `/path/to/script.sh` |

## 🔒 Security Configuration

### Access Control

```ini
[global]
# Enable access control
access_enabled = true

# Allow specific hosts
access_allowed_hosts = 127.0.0.1, 192.168.1.0/24, 10.0.0.0/8

# Allow specific networks
access_allowed_networks = 192.168.0.0/16

# Deny specific hosts
access_denied_hosts = 192.168.1.100, 10.0.0.50

# Time-based access
access_allowed_hours = 09:00-17:00
access_allowed_days = monday-friday

# Rate limiting
access_rate_limit = 100
access_rate_limit_window = 3600
```

### SSL Certificate Generation

```bash
# Generate private key
openssl genrsa -out server.key 2048

# Generate certificate signing request
openssl req -new -key server.key -out server.csr

# Generate self-signed certificate
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

# Generate CA certificate
openssl req -new -x509 -days 365 -key ca.key -out ca.crt
```

### User Database (v0.3.0 Enhanced)

#### Password File Format

The password file supports both plain text (for backward compatibility) and hashed passwords:

```bash
# Create user database
sudo touch /etc/simple-rsyncd/users
sudo chmod 600 /etc/simple-rsyncd/users

# Add users with plain text passwords (automatically hashed on first use)
echo "admin:secure_password" | sudo tee -a /etc/simple-rsyncd/users
echo "user1:user_password" | sudo tee -a /etc/simple-rsyncd/users

# Add users with pre-hashed passwords (SHA-256 format)
# Format: sha256:salt:hash
echo "admin:sha256:abc123...:def456..." | sudo tee -a /etc/simple-rsyncd/users

# Set permissions
sudo chown rsync:rsync /etc/simple-rsyncd/users
```

#### Password Hashing

Passwords are automatically hashed using SHA-256 with salt:
- **Format**: `sha256:salt:hash`
- **Salt**: Random 16-byte hex string
- **Hash**: SHA-256 hash of salt + password

**Example:**
```
admin:sha256:a1b2c3d4e5f6g7h8:9i0j1k2l3m4n5o6p7q8r9s0t1u2v3w4x5y6z7a8b9c0d1e2f3
```

#### User Database Management (v0.3.0)

The daemon now supports a user database with CRUD operations:

```ini
[global]
# Use user database instead of password file
auth_user_database = /etc/simple-rsyncd/userdb
auth_password_file =  # Leave empty to use database
```

**User Database Features:**
- User creation with password policy validation
- Password updates with re-hashing
- User deletion
- Permission management
- Account locking/unlocking
- Password expiration tracking
- Failed login attempt tracking

## 🌍 Environment Variables (v0.3.0)

### Variable Substitution

Configuration files support environment variable substitution:

```ini
[global]
# Use environment variables in configuration
bind_address = ${BIND_ADDRESS:0.0.0.0}  # Use BIND_ADDRESS env var, default to 0.0.0.0
bind_port = ${BIND_PORT:873}             # Use BIND_PORT env var, default to 873
log_file = ${LOG_DIR}/simple-rsyncd.log  # Use LOG_DIR env var

[module:backup]
path = ${BACKUP_ROOT}/data               # Use BACKUP_ROOT env var
```

**Supported Syntax:**
- `${VAR}` - Use environment variable `VAR`
- `${VAR:default}` - Use `VAR` if set, otherwise use `default`
- `$VAR` - Simple form (no default value)

**Examples:**
```bash
# Set environment variables
export BIND_ADDRESS=192.168.1.100
export BIND_PORT=1873
export LOG_DIR=/var/log
export BACKUP_ROOT=/mnt/backup

# Start daemon (will use environment variables)
simple-rsyncd start
```

### Predefined Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `SIMPLE_RSYNCD_CONFIG` | Configuration file path | `/etc/simple-rsyncd/rsyncd.conf` |
| `SIMPLE_RSYNCD_DEBUG` | Enable debug mode | `0` |
| `SIMPLE_RSYNCD_LOG_LEVEL` | Log level | `info` |
| `SIMPLE_RSYNCD_BIND_ADDRESS` | Bind address | `0.0.0.0` |
| `SIMPLE_RSYNCD_BIND_PORT` | Bind port | `873` |

## 🌍 Environment-Specific Configuration

### Development Environment

```ini
# /etc/simple-rsyncd/rsyncd.conf
[global]
# Development settings
ssl_enabled = false
auth_enabled = false
log_level = debug
bind_address = 127.0.0.1

[module:dev]
path = /home/user/dev
comment = Development files
read_only = false
list = true
delete = true
overwrite = true
```

### Production Environment

```ini
# /etc/simple-rsyncd/rsyncd.conf
[global]
# Production settings
ssl_enabled = true
auth_enabled = true
log_level = warn
bind_address = 0.0.0.0
max_connections = 500

# Security
ssl_require_client_cert = true
access_enabled = true
access_allowed_hosts = 10.0.0.0/8, 192.168.0.0/16

[module:production]
path = /var/production
comment = Production data
read_only = false
list = false
delete = false
overwrite = true
allowed_users = admin,prod_user
```

### Testing Environment

```ini
# /etc/simple-rsyncd/rsyncd.conf
[global]
# Testing settings
ssl_enabled = false
auth_enabled = false
log_level = debug
bind_address = 127.0.0.1
bind_port = 1873

[module:test]
path = /tmp/test
comment = Test files
read_only = false
list = true
delete = true
overwrite = true
```

## 📊 Monitoring Configuration

### Metrics and Health Checks

```ini
[global]
# Monitoring settings
monitoring_enabled = true
monitoring_port = 9090
monitoring_bind_address = 127.0.0.1

# Health checks
health_check_enabled = true
health_check_interval = 30
health_check_timeout = 5

# Prometheus metrics
prometheus_enabled = true
prometheus_path = /metrics
```

### Logging and Monitoring

```ini
[global]
# Logging
log_level = info
log_file = /var/log/simple-rsyncd.log
log_format = json
log_output = file,syslog

# Log rotation
log_max_size = 100MB
log_max_files = 5

# Structured logging
log_include_timestamp = true
log_include_hostname = true
log_include_process_id = true
```

## 🚀 Performance Tuning

### Transfer Optimization

```ini
[global]
# Buffer settings
buffer_size = 16384
max_buffer_count = 100

# Compression
compression_enabled = true
compression_level = 6
compression_min_size = 1024

# Transfer limits
max_transfer_rate = 0
max_file_size = 0
max_directory_depth = 100

# Connection pooling
connection_pool_size = 10
connection_pool_timeout = 300
```

### Memory and Resource Management

```ini
[global]
# Memory limits
max_memory = 1024
max_memory_per_connection = 64

# Worker threads
worker_threads = 8
worker_thread_pool_size = 20

# File handling
max_open_files = 1000
file_lock_timeout = 30

# Cache settings
cache_enabled = true
cache_size = 100
cache_ttl = 300
```

## 🔧 Advanced Configuration

### Configuration Inheritance

```ini
# Base configuration
[global]
ssl_enabled = true
auth_enabled = true
log_level = info

# Production override
[global:production]
ssl_require_client_cert = true
auth_method = oauth2
log_level = warn
max_connections = 1000

# Development override
[global:development]
ssl_enabled = false
auth_enabled = false
log_level = debug
bind_address = 127.0.0.1
```

### Dynamic Configuration

```ini
[global]
# Dynamic configuration
config_reload_enabled = true
config_reload_interval = 300
config_reload_signal = SIGHUP

# Hot reloading
hot_reload_enabled = true
hot_reload_modules = true
hot_reload_ssl = false

# Configuration validation
config_validation_enabled = true
config_validation_strict = false
```

### Script Integration (v0.3.0)

Module script hooks are executed before and after operations:

```ini
[module:backup]
# Pre-operation scripts
pre_transfer_script = /usr/local/bin/pre-backup.sh
pre_delete_script = /usr/local/bin/pre-delete.sh
pre_list_script = /usr/local/bin/pre-list.sh

# Post-operation scripts
post_transfer_script = /usr/local/bin/post-backup.sh
post_delete_script = /usr/local/bin/post-delete.sh
post_list_script = /usr/local/bin/post-list.sh

# Script environment variables (passed to scripts)
# Available variables:
# - RSYNC_MODULE: Module name
# - RSYNC_OPERATION: Operation type (transfer, delete, list)
# - RSYNC_PATH: File/directory path
# - RSYNC_USER: Authenticated username
# - RSYNC_CLIENT: Client IP address
```

**Script Execution:**
- Scripts receive environment variables with operation context
- Scripts can prevent operations by returning non-zero exit code
- Script output is logged for debugging
- Script failures are logged but don't necessarily stop operations (configurable)

**Example Script:**
```bash
#!/bin/bash
# pre_transfer_script example

echo "Pre-transfer hook for module: $RSYNC_MODULE"
echo "Operation: $RSYNC_OPERATION"
echo "Path: $RSYNC_PATH"
echo "User: $RSYNC_USER"

# Perform pre-transfer checks
if [ ! -d "/backup/staging" ]; then
    echo "Error: Staging directory not available"
    exit 1
fi

exit 0
```

## 📝 JSON Configuration Format (v0.3.0)

The daemon supports JSON configuration files as an alternative to INI format:

```json
{
  "global": {
    "bind_address": "0.0.0.0",
    "bind_port": 873,
    "max_connections": 100,
    "ssl": {
      "enabled": true,
      "certificate_file": "/etc/simple-rsyncd/ssl/server.crt",
      "private_key_file": "/etc/simple-rsyncd/ssl/server.key"
    },
    "auth": {
      "enabled": true,
      "method": "password",
      "password_file": "/etc/simple-rsyncd/users",
      "password_policy": {
        "min_length": 8,
        "require_uppercase": true,
        "require_lowercase": true,
        "require_digits": true,
        "expiration_hours": 2160
      }
    },
    "log": {
      "level": "info",
      "file": "/var/log/simple-rsyncd.log",
      "format": "json",
      "max_size": 10485760,
      "max_files": 5
    },
    "auto_reload": true,
    "reload_interval": 30
  },
  "modules": {
    "public": {
      "path": "/var/public",
      "comment": "Public files",
      "read_only": true,
      "list": true,
      "include_patterns": ["*.txt", "*.pdf"],
      "exclude_patterns": ["*.tmp"]
    },
    "backup": {
      "path": "/var/backup",
      "comment": "Backup storage",
      "read_only": false,
      "list": true,
      "delete": true,
      "pre_transfer_script": "/usr/local/bin/pre-backup.sh",
      "post_transfer_script": "/usr/local/bin/post-backup.sh"
    }
  }
}
```

**JSON Configuration Benefits:**
- Structured and hierarchical
- Better for programmatic generation
- Supports nested objects and arrays
- Easier to validate with JSON schema

## 📝 Configuration Examples

### Minimal Configuration

```ini
[global]
bind_address = 0.0.0.0
bind_port = 873

[module:public]
path = /var/public
comment = Public files
read_only = true
```

### Secure Configuration

```ini
[global]
bind_address = 0.0.0.0
bind_port = 873
ssl_enabled = true
ssl_certificate_file = /etc/simple-rsyncd/ssl/server.crt
ssl_private_key_file = /etc/simple-rsyncd/ssl/server.key
auth_enabled = true
auth_method = password
auth_password_file = /etc/simple-rsyncd/users
access_enabled = true
access_allowed_hosts = 192.168.1.0/24
log_level = warn
log_file = /var/log/simple-rsyncd.log

[module:secure]
path = /var/secure
comment = Secure files
read_only = false
list = false
delete = true
overwrite = true
allowed_users = admin
```

### High-Performance Configuration

```ini
[global]
bind_address = 0.0.0.0
bind_port = 873
max_connections = 1000
worker_threads = 16
buffer_size = 32768
compression_enabled = true
compression_level = 6
cache_enabled = true
cache_size = 500
log_level = info

[module:fast]
path = /var/fast
comment = High-performance storage
read_only = false
list = true
delete = true
overwrite = true
```

## 📋 JSON Configuration Examples (v0.3.0)

### Minimal JSON Configuration

```json
{
  "global": {
    "bind_address": "0.0.0.0",
    "bind_port": 873
  },
  "modules": {
    "public": {
      "path": "/var/public",
      "comment": "Public files",
      "read_only": true
    }
  }
}
```

### Complete JSON Configuration

```json
{
  "global": {
    "bind_address": "0.0.0.0",
    "bind_port": 873,
    "max_connections": 100,
    "auto_reload": true,
    "reload_interval": 30,
    "network": {
      "bind_address": "0.0.0.0",
      "bind_port": 873,
      "max_connections": 100
    },
    "ssl": {
      "enabled": true,
      "certificate_file": "/etc/simple-rsyncd/ssl/server.crt",
      "private_key_file": "/etc/simple-rsyncd/ssl/server.key"
    },
    "auth": {
      "enabled": true,
      "method": "password",
      "password_file": "/etc/simple-rsyncd/users",
      "password_policy": {
        "min_length": 8,
        "require_uppercase": true,
        "require_lowercase": true,
        "require_digits": true,
        "expiration_hours": 2160
      },
      "session_timeout": 3600
    },
    "log": {
      "level": "info",
      "file": "/var/log/simple-rsyncd.log",
      "format": "json",
      "max_size": 10485760,
      "max_files": 5,
      "compress_old_logs": true
    }
  },
  "modules": {
    "backup": {
      "path": "/var/backup",
      "comment": "Backup storage",
      "read_only": false,
      "list": true,
      "delete": true,
      "include_patterns": ["*.tar.gz", "*.sql"],
      "exclude_patterns": ["*.tmp"],
      "pre_transfer_script": "/usr/local/bin/pre-backup.sh",
      "post_transfer_script": "/usr/local/bin/post-backup.sh"
    }
  }
}
```

### Using Environment Variables in JSON

```json
{
  "global": {
    "bind_address": "${BIND_ADDRESS:0.0.0.0}",
    "bind_port": ${BIND_PORT:873},
    "log": {
      "file": "${LOG_DIR}/simple-rsyncd.log"
    }
  },
  "modules": {
    "backup": {
      "path": "${BACKUP_ROOT}/data"
    }
  }
}
```

**Note**: JSON format does not support environment variable substitution in the same way as INI format. For JSON, use environment variables as runtime overrides or pre-process the JSON file.

## ✅ Configuration Validation

### Validation Commands

```bash
# Test configuration syntax
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

# Test JSON configuration
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.json

# Validate configuration
simple-rsyncd validate --config /etc/simple-rsyncd/rsyncd.conf

# Check for security issues
simple-rsyncd validate --config /etc/simple-rsyncd/rsyncd.conf --security

# Validate specific module
simple-rsyncd validate --config /etc/simple-rsyncd/rsyncd.conf --module backup
```

### Common Validation Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `Invalid path` | Path doesn't exist or inaccessible | Create directory and set permissions |
| `SSL certificate not found` | Certificate file missing | Generate or copy certificate files |
| `Invalid user` | User doesn't exist | Create user or check user database |
| `Permission denied` | Insufficient permissions | Check file and directory permissions |
| `Port already in use` | Port 873 occupied | Change port or stop conflicting service |

## 🔄 Configuration Reloading (v0.3.0 Enhanced)

### Automatic Hot-Reload

The daemon can automatically monitor and reload configuration files when they change:

```ini
[global]
# Enable automatic configuration reloading
auto_reload = true

# Check for changes every 30 seconds
reload_interval = 30
```

**How it works:**
- The daemon monitors the configuration file for changes
- When a change is detected, it validates the new configuration
- If valid, the configuration is reloaded without restarting the daemon
- If invalid, the old configuration is kept and an error is logged
- Modules are automatically reloaded when configuration changes

### Manual Reload Commands

```bash
# Reload all configuration
simple-rsyncd reload

# Reload specific module
simple-rsyncd reload --module backup

# Validate before reload
simple-rsyncd reload --validate

# Force reload
simple-rsyncd reload --force
```

### Signal-Based Reloading

```bash
# Send SIGHUP to reload
kill -HUP $(cat /var/run/simple-rsyncd.pid)

# Send SIGUSR1 to reload modules only
kill -USR1 $(cat /var/run/simple-rsyncd.pid)

# Send SIGUSR2 to reload SSL certificates
kill -USR2 $(cat /var/run/simple-rsyncd.pid)
```

### Reload Behavior

- **Network settings**: Require daemon restart (bind address/port changes)
- **SSL certificates**: Can be reloaded without restart
- **Authentication**: Can be reloaded without restart
- **Module configuration**: Can be reloaded without restart
- **Logging settings**: Can be reloaded without restart

## 📚 Next Steps

After configuring your daemon:

1. **Test the configuration**: Use validation commands
2. **Start the daemon**: See [User Guide](../user-guide/README.md)
3. **Set up monitoring**: Configure metrics and health checks
4. **Secure the installation**: Enable SSL and authentication
5. **Optimize performance**: Tune buffer sizes and worker threads

---

**Configuration complete?** Move on to the [User Guide](../user-guide/README.md) to learn how to use your configured daemon.
