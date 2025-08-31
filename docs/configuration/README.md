# Configuration Guide

This guide covers all configuration options for Simple RSync Daemon, from basic setup to advanced features.

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
├── rsyncd.conf              # Main configuration file
├── modules.d/               # Module configuration files
│   ├── 01-public.conf      # Public module
│   ├── 02-backup.conf      # Backup module
│   └── 03-private.conf     # Private module
├── ssl/                     # SSL certificates
│   ├── server.crt          # Server certificate
│   ├── server.key          # Server private key
│   └── ca.crt              # CA certificate
├── users                    # User database
└── templates/               # Configuration templates
```

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

# Logging
log_level = info
log_file = /var/log/simple-rsyncd.log
log_format = json

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

### Logging Configuration

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `log_level` | Log level | `info` | `debug` |
| `log_file` | Log file path | - | `/var/log/simple-rsyncd.log` |
| `log_format` | Log format | `text` | `json` |
| `log_max_size` | Max log file size | `100MB` | `500MB` |
| `log_max_files` | Max log files to keep | `5` | `10` |
| `log_output` | Log destinations | `file` | `file,syslog` |

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

### User Database

```bash
# Create user database
sudo touch /etc/simple-rsyncd/users
sudo chmod 600 /etc/simple-rsyncd/users

# Add users (username:password)
echo "admin:$(openssl passwd -1 'secure_password')" | sudo tee -a /etc/simple-rsyncd/users
echo "user1:$(openssl passwd -1 'user_password')" | sudo tee -a /etc/simple-rsyncd/users

# Set permissions
sudo chown rsync:rsync /etc/simple-rsyncd/users
```

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

### Script Integration

```ini
[module:backup]
# Script hooks
pre_transfer_script = /usr/local/bin/pre-backup.sh
post_transfer_script = /usr/local/bin/post-backup.sh
delete_script = /usr/local/bin/pre-delete.sh
list_script = /usr/local/bin/pre-list.sh

# Script environment
script_timeout = 30
script_user = rsync
script_group = rsync

# Script logging
script_log_enabled = true
script_log_file = /var/log/simple-rsyncd-scripts.log
```

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

## ✅ Configuration Validation

### Validation Commands

```bash
# Test configuration syntax
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

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

## 🔄 Configuration Reloading

### Reload Commands

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

## 📚 Next Steps

After configuring your daemon:

1. **Test the configuration**: Use validation commands
2. **Start the daemon**: See [User Guide](../user-guide/README.md)
3. **Set up monitoring**: Configure metrics and health checks
4. **Secure the installation**: Enable SSL and authentication
5. **Optimize performance**: Tune buffer sizes and worker threads

---

**Configuration complete?** Move on to the [User Guide](../user-guide/README.md) to learn how to use your configured daemon.
