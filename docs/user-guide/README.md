# User Guide

This guide covers everything you need to know to use Simple RSync Daemon effectively.

## 🚀 Quick Start

### First Run

```bash
# Start the daemon with default configuration
simple-rsyncd start

# Start as a background daemon
simple-rsyncd start --daemon

# Start with custom configuration
simple-rsyncd start --config /path/to/rsyncd.conf
```

### Basic Test

```bash
# List available modules
rsync rsync://localhost/

# Test file transfer
echo "Hello World" > test.txt
rsync -avz test.txt rsync://localhost/public/
```

## 📋 Command Reference

### Daemon Control Commands

#### Start the Daemon

```bash
# Start in foreground
simple-rsyncd start [options]

# Start in background
simple-rsyncd start --daemon [options]

# Start with specific configuration
simple-rsyncd start --config /etc/simple-rsyncd/rsyncd.conf

# Start with custom port
simple-rsyncd start --port 1873

# Start with debug logging
simple-rsyncd start --debug
```

#### Stop the Daemon

```bash
# Stop gracefully
simple-rsyncd stop

# Force stop
simple-rsyncd stop --force

# Stop by PID file
simple-rsyncd stop --pid-file /var/run/simple-rsyncd.pid
```

#### Daemon Status

```bash
# Check daemon status
simple-rsyncd status

# Show detailed status
simple-rsyncd status --verbose

# Check specific module
simple-rsyncd status --module public
```

#### Reload Configuration

```bash
# Reload configuration without restart
simple-rsyncd reload

# Reload specific module
simple-rsyncd reload --module backup

# Validate configuration before reload
simple-rsyncd reload --validate
```

#### Restart the Daemon

```bash
# Restart daemon
simple-rsyncd restart

# Restart with new configuration
simple-rsyncd restart --config /new/config.conf
```

### Configuration Commands

#### Test Configuration

```bash
# Test configuration syntax
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

# Test with verbose output
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf --verbose

# Test specific module
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf --module backup
```

#### Validate Configuration

```bash
# Validate configuration
simple-rsyncd validate --config /etc/simple-rsyncd/rsyncd.conf

# Check for security issues
simple-rsyncd validate --config /etc/simple-rsyncd/rsyncd.conf --security
```

### Information Commands

#### Version Information

```bash
# Show version
simple-rsyncd --version

# Show build information
simple-rsyncd --version --verbose
```

#### Help

```bash
# Show help
simple-rsyncd --help

# Show help for specific command
simple-rsyncd start --help
```

## 🔧 Configuration Management

### Configuration File Structure

```ini
# /etc/simple-rsyncd/rsyncd.conf

[global]
# Global settings
bind_address = 0.0.0.0
bind_port = 873
ssl_enabled = true
auth_enabled = true

[module:public]
# Module configuration
path = /var/lib/simple-rsyncd/public
comment = Public files
read_only = true
list = true

[module:backup]
# Another module
path = /var/backup
comment = Backup storage
read_only = false
list = true
delete = true
```

### Environment Variables

```bash
# Override configuration file location
export SIMPLE_RSYNCD_CONFIG=/path/to/config

# Enable debug mode
export SIMPLE_RSYNCD_DEBUG=1

# Set log level
export SIMPLE_RSYNCD_LOG_LEVEL=debug

# Override bind address
export SIMPLE_RSYNCD_BIND_ADDRESS=127.0.0.1

# Override bind port
export SIMPLE_RSYNCD_BIND_PORT=1873
```

### Configuration Inheritance

```ini
# Base configuration
[global]
ssl_enabled = true
auth_enabled = true

# Production override
[global:production]
ssl_require_client_cert = true
auth_method = oauth2

# Development override
[global:development]
ssl_enabled = false
auth_enabled = false
```

## 📁 Module Management

### Creating Modules

```ini
# Basic module
[module:documents]
path = /var/documents
comment = Document storage
read_only = false
list = true
delete = true
overwrite = true

# Read-only module
[module:public]
path = /var/public
comment = Public files
read_only = true
list = true
delete = false
overwrite = false

# Restricted module
[module:private]
path = /var/private
comment = Private files
read_only = false
list = false
delete = true
overwrite = true
```

### Module Permissions

| Permission | Description | Default |
|------------|-------------|---------|
| `read_only` | Module is read-only | `false` |
| `list` | Allow directory listing | `true` |
| `delete` | Allow file deletion | `false` |
| `overwrite` | Allow file overwriting | `false` |
| `create` | Allow file creation | `true` |

### Module Patterns

```ini
[module:backup]
path = /var/backup
comment = Backup storage

# Include patterns
include = *.tar.gz
include = *.sql
include = */backup/*

# Exclude patterns
exclude = *.tmp
exclude = */temp/*
exclude = .DS_Store
```

## 🔒 Security Configuration

### SSL/TLS Setup

```ini
[global]
ssl_enabled = true
ssl_certificate_file = /etc/simple-rsyncd/ssl/server.crt
ssl_private_key_file = /etc/simple-rsyncd/ssl/server.key
ssl_ca_file = /etc/simple-rsyncd/ssl/ca.crt
ssl_cipher_suite = ECDHE-RSA-AES256-GCM-SHA384
ssl_tls_version = 1.2
ssl_require_client_cert = false
```

### Authentication

```ini
[global]
auth_enabled = true
auth_method = password
auth_password_file = /etc/simple-rsyncd/users
auth_realm = simple-rsyncd

# OAuth2 authentication
auth_method = oauth2
auth_oauth2_client_id = your_client_id
auth_oauth2_client_secret = your_client_secret
auth_oauth2_issuer = https://accounts.google.com
```

### Access Control

```ini
[global]
access_enabled = true

# Allow specific hosts
access_allowed_hosts = 127.0.0.1, 192.168.1.0/24

# Allow specific networks
access_allowed_networks = 10.0.0.0/8

# Deny specific hosts
access_denied_hosts = 192.168.1.100

# Time-based access
access_allowed_hours = 09:00-17:00
```

## 📊 Monitoring and Logging

### Logging Configuration

```ini
[global]
# Log level
log_level = info

# Log file
log_file = /var/log/simple-rsyncd.log

# Log rotation
log_max_size = 100MB
log_max_files = 5

# Log format
log_format = json

# Log destinations
log_output = file,syslog
```

### Metrics and Health Checks

```bash
# Check daemon health
simple-rsyncd health

# Show metrics
simple-rsyncd metrics

# Export metrics for Prometheus
simple-rsyncd metrics --format prometheus

# Health check endpoint
curl http://localhost:9090/health
```

### Performance Monitoring

```bash
# Show connection statistics
simple-rsyncd stats --connections

# Show transfer statistics
simple-rsyncd stats --transfers

# Show performance metrics
simple-rsyncd stats --performance
```

## 🌐 Client Usage

### Basic rsync Commands

```bash
# List available modules
rsync rsync://server/

# List module contents
rsync rsync://server/module/

# Upload files
rsync -avz /local/path/ rsync://server/module/

# Download files
rsync -avz rsync://server/module/ /local/path/

# Sync directories
rsync -avz --delete /local/path/ rsync://server/module/
```

### Authentication

```bash
# With password file
rsync -avz --password-file=/path/to/password /local/path/ rsync://user@server/module/

# With OAuth2 token
rsync -avz -H "Authorization: Bearer $TOKEN" /local/path/ rsync://server/module/
```

### SSL/TLS Connections

```bash
# With SSL verification
rsync -avz -e "rsync --rsh='openssl s_client -quiet -connect server:873'" /local/path/ /module/

# Skip SSL verification (not recommended for production)
rsync -avz -e "rsync --rsh='openssl s_client -quiet -connect server:873 -verify_return_error'" /local/path/ /module/
```

### Advanced Options

```bash
# Bandwidth limiting
rsync -avz --bwlimit=1000 /local/path/ rsync://server/module/

# Compression
rsync -avz --compress-level=9 /local/path/ rsync://server/module/

# Partial transfers
rsync -avz --partial /local/path/ rsync://server/module/

# Progress display
rsync -avz --progress /local/path/ rsync://server/module/
```

## 🐳 Docker Usage

### Running in Docker

```bash
# Run with default configuration
docker run -d \
    --name simple-rsyncd \
    -p 873:873 \
    simple-rsyncd/simple-rsyncd:latest

# Run with custom configuration
docker run -d \
    --name simple-rsyncd \
    -p 873:873 \
    -v /path/to/config:/etc/simple-rsyncd \
    -v /path/to/data:/var/lib/simple-rsyncd \
    simple-rsyncd/simple-rsyncd:latest
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

## 🔧 Service Management

### Systemd Service (Linux)

```bash
# Enable service
sudo systemctl enable simple-rsyncd

# Start service
sudo systemctl start simple-rsyncd

# Check status
sudo systemctl status simple-rsyncd

# Stop service
sudo systemctl stop simple-rsyncd

# Restart service
sudo systemctl restart simple-rsyncd

# Reload configuration
sudo systemctl reload simple-rsyncd
```

### Launchd Service (macOS)

```bash
# Create service file
sudo tee /Library/LaunchDaemons/com.simple-rsyncd.plist > /dev/null <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.simple-rsyncd</string>
    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/simple-rsyncd</string>
        <string>start</string>
        <string>--daemon</string>
        <string>--config</string>
        <string>/etc/simple-rsyncd/rsyncd.conf</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
</dict>
</plist>
EOF

# Load service
sudo launchctl load /Library/LaunchDaemons/com.simple-rsyncd.plist
```

## 📚 Examples

### Backup Script

```bash
#!/bin/bash
# Daily backup script

BACKUP_DIR="/var/backup"
DATE=$(date +%Y%m%d)
MODULE="backup"

# Create backup
rsync -avz --delete \
    --exclude="*.tmp" \
    --exclude=".cache" \
    /home/user/ \
    rsync://localhost/$MODULE/daily-$DATE/

# Clean old backups (keep 7 days)
find $BACKUP_DIR -name "daily-*" -mtime +7 -exec rm -rf {} \;
```

### Sync Script

```bash
#!/bin/bash
# Sync script with error handling

SOURCE="/local/data"
DEST="rsync://server/backup"
LOG_FILE="/var/log/sync.log"

echo "$(date): Starting sync" >> $LOG_FILE

if rsync -avz --delete "$SOURCE" "$DEST" >> $LOG_FILE 2>&1; then
    echo "$(date): Sync completed successfully" >> $LOG_FILE
    exit 0
else
    echo "$(date): Sync failed" >> $LOG_FILE
    exit 1
fi
```

## 🚨 Troubleshooting

### Common Issues

**Daemon won't start**
```bash
# Check configuration
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

# Check logs
tail -f /var/log/simple-rsyncd.log

# Check port availability
netstat -tlnp | grep :873
```

**Connection refused**
```bash
# Check daemon status
simple-rsyncd status

# Check firewall
sudo ufw status
sudo iptables -L

# Check bind address
netstat -tlnp | grep simple-rsyncd
```

**Authentication failed**
```bash
# Check user database
cat /etc/simple-rsyncd/users

# Check authentication method
grep auth_method /etc/simple-rsyncd/rsyncd.conf

# Check logs for auth errors
grep -i auth /var/log/simple-rsyncd.log
```

## 📖 Next Steps

After mastering basic usage:

1. **Advanced Configuration**: See [Configuration Guide](../configuration/README.md)
2. **Security Hardening**: See [Security Configuration](../configuration/README.md#security)
3. **Performance Tuning**: See [Performance Guide](../configuration/README.md#performance)
4. **API Development**: See [API Reference](../api/README.md)

---

**Need help?** Check the [Troubleshooting Guide](../troubleshooting/README.md) for common issues and solutions.
