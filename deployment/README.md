# simple-rsyncd Deployment

This directory contains deployment configurations and examples for simple-rsyncd.

## Directory Structure

```
deployment/
├── systemd/                    # Linux systemd service files
│   └── simple-rsyncd.service
├── launchd/                    # macOS launchd service files
│   └── com.simple-rsyncd.simple-rsyncd.plist
├── logrotate.d/                # Linux log rotation configuration
│   └── simple-rsyncd
├── windows/                    # Windows service management
│   └── simple-rsyncd.service.bat
└── examples/                   # Deployment examples
    └── docker/                 # Docker deployment examples
        ├── docker-compose.yml
        └── README.md
```

## Platform-Specific Deployment

### Linux (systemd)

1. **Install the service file:**
   ```bash
   sudo cp deployment/systemd/simple-rsyncd.service /etc/systemd/system/
   sudo systemctl daemon-reload
   ```

2. **Create user and group:**
   ```bash
   sudo useradd --system --no-create-home --shell /bin/false simple-rsyncd
   ```

3. **Enable and start the service:**
   ```bash
   sudo systemctl enable simple-rsyncd
   sudo systemctl start simple-rsyncd
   ```

4. **Check status:**
   ```bash
   sudo systemctl status simple-rsyncd
   sudo journalctl -u simple-rsyncd -f
   ```

### macOS (launchd)

1. **Install the plist file:**
   ```bash
   sudo cp deployment/launchd/com.simple-rsyncd.simple-rsyncd.plist /Library/LaunchDaemons/
   sudo chown root:wheel /Library/LaunchDaemons/com.simple-rsyncd.simple-rsyncd.plist
   ```

2. **Load and start the service:**
   ```bash
   sudo launchctl load /Library/LaunchDaemons/com.simple-rsyncd.simple-rsyncd.plist
   sudo launchctl start com.simple-rsyncd.simple-rsyncd
   ```

3. **Check status:**
   ```bash
   sudo launchctl list | grep simple-rsyncd
   tail -f /var/log/simple-rsyncd.log
   ```

### Windows

1. **Run as Administrator:**
   ```cmd
   # Install service
   deployment\windows\simple-rsyncd.service.bat install
   
   # Start service
   deployment\windows\simple-rsyncd.service.bat start
   
   # Check status
   deployment\windows\simple-rsyncd.service.bat status
   ```

2. **Service management:**
   ```cmd
   # Stop service
   deployment\windows\simple-rsyncd.service.bat stop
   
   # Restart service
   deployment\windows\simple-rsyncd.service.bat restart
   
   # Uninstall service
   deployment\windows\simple-rsyncd.service.bat uninstall
   ```

## Log Rotation (Linux)

1. **Install logrotate configuration:**
   ```bash
   sudo cp deployment/logrotate.d/simple-rsyncd /etc/logrotate.d/
   ```

2. **Test logrotate configuration:**
   ```bash
   sudo logrotate -d /etc/logrotate.d/simple-rsyncd
   ```

3. **Force log rotation:**
   ```bash
   sudo logrotate -f /etc/logrotate.d/simple-rsyncd
   ```

## Docker Deployment

See [examples/docker/README.md](examples/docker/README.md) for detailed Docker deployment instructions.

### Quick Start

```bash
# Build and run with Docker Compose
cd deployment/examples/docker
docker-compose up -d

# Check status
docker-compose ps
docker-compose logs simple-rsyncd
```

## Configuration

### Service Configuration

Each platform has specific configuration requirements:

- **Linux**: Edit `/etc/systemd/system/simple-rsyncd.service`
- **macOS**: Edit `/Library/LaunchDaemons/com.simple-rsyncd.simple-rsyncd.plist`
- **Windows**: Edit the service binary path in the batch file

### Application Configuration

Place your application configuration in:
- **Linux/macOS**: `/etc/simple-rsyncd/simple-rsyncd.conf`
- **Windows**: `%PROGRAMFILES%\simple-rsyncd\simple-rsyncd.conf`

## Security Considerations

### User and Permissions

1. **Create dedicated user:**
   ```bash
   # Linux
   sudo useradd --system --no-create-home --shell /bin/false simple-rsyncd
   
   # macOS
   sudo dscl . -create /Users/_simple-rsyncd UserShell /usr/bin/false
   sudo dscl . -create /Users/_simple-rsyncd UniqueID 200
   sudo dscl . -create /Users/_simple-rsyncd PrimaryGroupID 200
   sudo dscl . -create /Groups/_simple-rsyncd GroupID 200
   ```

2. **Set proper permissions:**
   ```bash
   # Configuration files
   sudo chown root:simple-rsyncd /etc/simple-rsyncd/simple-rsyncd.conf
   sudo chmod 640 /etc/simple-rsyncd/simple-rsyncd.conf
   
   # Log files
   sudo chown simple-rsyncd:simple-rsyncd /var/log/simple-rsyncd/
   sudo chmod 755 /var/log/simple-rsyncd/
   ```

### Firewall Configuration

Configure firewall rules as needed:

```bash
# Linux (ufw)
sudo ufw allow 873/tcp

# Linux (firewalld)
sudo firewall-cmd --permanent --add-port=873/tcp
sudo firewall-cmd --reload

# macOS
sudo pfctl -f /etc/pf.conf
```

## Monitoring

### Health Checks

1. **Service status:**
   ```bash
   # Linux
   sudo systemctl is-active simple-rsyncd
   
   # macOS
   sudo launchctl list | grep simple-rsyncd
   
   # Windows
   sc query simple-rsyncd
   ```

2. **Port availability:**
   ```bash
   netstat -tlnp | grep 873
   ss -tlnp | grep 873
   ```

3. **Process monitoring:**
   ```bash
   ps aux | grep simple-rsyncd
   top -p $(pgrep simple-rsyncd)
   ```

### Log Monitoring

1. **Real-time logs:**
   ```bash
   # Linux
   sudo journalctl -u simple-rsyncd -f
   
   # macOS
   tail -f /var/log/simple-rsyncd.log
   
   # Windows
   # Use Event Viewer or PowerShell Get-WinEvent
   ```

2. **Log analysis:**
   ```bash
   # Search for errors
   sudo journalctl -u simple-rsyncd --since "1 hour ago" | grep -i error
   
   # Count log entries
   sudo journalctl -u simple-rsyncd --since "1 day ago" | wc -l
   ```

## Troubleshooting

### Common Issues

1. **Service won't start:**
   - Check configuration file syntax
   - Verify user permissions
   - Check port availability
   - Review service logs

2. **Permission denied:**
   - Ensure service user exists
   - Check file permissions
   - Verify directory ownership

3. **Port already in use:**
   - Check what's using the port: `netstat -tlnp | grep 873`
   - Stop conflicting service or change port

4. **Service stops unexpectedly:**
   - Check application logs
   - Verify resource limits
   - Review system logs

### Debug Mode

Run the service in debug mode for troubleshooting:

```bash
# Linux/macOS
sudo -u simple-rsyncd /usr/local/bin/simple-rsyncd --debug

# Windows
simple-rsyncd.exe --debug
```

### Log Levels

Adjust log level for more verbose output:

```bash
# Set log level in configuration
log_level = debug

# Or via environment variable
export SIMPLE-RSYNCD_LOG_LEVEL=debug
```

## Backup and Recovery

### Configuration Backup

```bash
# Backup configuration
sudo tar -czf simple-rsyncd-config-backup-$(date +%Y%m%d).tar.gz /etc/simple-rsyncd/

# Backup logs
sudo tar -czf simple-rsyncd-logs-backup-$(date +%Y%m%d).tar.gz /var/log/simple-rsyncd/
```

### Service Recovery

```bash
# Stop service
sudo systemctl stop simple-rsyncd

# Restore configuration
sudo tar -xzf simple-rsyncd-config-backup-YYYYMMDD.tar.gz -C /

# Start service
sudo systemctl start simple-rsyncd
```

## Updates

### Service Update Process

1. **Stop service:**
   ```bash
   sudo systemctl stop simple-rsyncd
   ```

2. **Backup current version:**
   ```bash
   sudo cp /usr/local/bin/simple-rsyncd /usr/local/bin/simple-rsyncd.backup
   ```

3. **Install new version:**
   ```bash
   sudo cp simple-rsyncd /usr/local/bin/
   sudo chmod +x /usr/local/bin/simple-rsyncd
   ```

4. **Start service:**
   ```bash
   sudo systemctl start simple-rsyncd
   ```

5. **Verify update:**
   ```bash
   sudo systemctl status simple-rsyncd
   simple-rsyncd --version
   ```
