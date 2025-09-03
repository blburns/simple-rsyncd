# Advanced Configuration Examples

This directory contains advanced configuration examples for simple-rsyncd optimized for specific use cases and high-performance environments.

## Configuration Files

### `high-performance.conf`
- **Purpose**: Optimized for high-traffic environments
- **Features**:
  - High connection limits (1000 max connections)
  - SSL/TLS encryption
  - Authentication enabled
  - Advanced performance tuning
  - Prometheus metrics
- **Use Case**: High-traffic file servers, content distribution
- **Modules**: `high-speed`, `bulk-transfer`, `streaming`

### `high-security.conf`
- **Purpose**: Enhanced security for sensitive environments
- **Features**:
  - Strict access controls
  - Client certificate authentication
  - Enhanced logging with client IP tracking
  - Rate limiting
  - Chroot security
- **Use Case**: Sensitive data, compliance environments
- **Modules**: `secure-documents`, `confidential`, `audit-logs`

### `load-balanced.conf`
- **Purpose**: Multi-instance load-balanced deployment
- **Features**:
  - Moderate connection limits for load balancing
  - SSL/TLS encryption
  - Authentication enabled
  - Instance-aware logging
  - Performance optimization
- **Use Case**: Load-balanced deployments, CDN backends
- **Modules**: `web-content`, `software-repo`, `media-cdn`, `backup-sync`

## Prerequisites

### SSL/TLS Certificates
For configurations with SSL enabled:
```bash
# Generate self-signed certificate (development)
openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes

# Or use Let's Encrypt (production)
certbot certonly --standalone -d your-domain.com
```

### Authentication Setup
For configurations with authentication:
```bash
# Create user database
htpasswd -c /etc/simple-rsyncd/users username

# Or create public key file
ssh-keygen -t rsa -b 4096 -f /etc/simple-rsyncd/keys/user_key
```

### Scripts
Advanced configurations use pre/post transfer scripts:
```bash
# Create script directory
mkdir -p /usr/local/bin

# Make scripts executable
chmod +x /usr/local/bin/pre-*.sh
chmod +x /usr/local/bin/post-*.sh
```

## Deployment

1. **Choose appropriate configuration:**
   ```bash
   cp config/examples/advanced/high-performance.conf /etc/simple-rsyncd/rsyncd.conf
   ```

2. **Set up prerequisites:**
   - SSL certificates
   - Authentication files
   - Transfer scripts
   - Directory structure

3. **Create directories:**
   ```bash
   mkdir -p /var/lib/simple-rsyncd/{high-speed,bulk,streaming}
   mkdir -p /var/log/simple-rsyncd
   chown -R rsync:rsync /var/lib/simple-rsyncd /var/log/simple-rsyncd
   ```

4. **Start the daemon:**
   ```bash
   simple-rsyncd start --config /etc/simple-rsyncd/rsyncd.conf
   ```

## Performance Tuning

### High Performance Settings
- **Buffer Size**: 256KB for high-speed transfers
- **Compression**: Level 9 for maximum compression
- **Concurrent Transfers**: 50 simultaneous transfers
- **Pipeline Depth**: 10 for optimal throughput

### Security Settings
- **Access Control**: Strict IP-based restrictions
- **Authentication**: Client certificate required
- **Logging**: Comprehensive audit trail
- **Rate Limiting**: Aggressive connection limits

### Load Balancing Settings
- **Connection Limits**: Balanced for multiple instances
- **SSL/TLS**: Required for secure communication
- **Monitoring**: Instance-aware metrics
- **Performance**: Optimized for distributed workloads

## Monitoring

### Prometheus Metrics
Enable Prometheus metrics in configuration:
```ini
monitoring_enable_prometheus_metrics = true
monitoring_prometheus_port = 9090
```

### Health Checks
Monitor daemon health:
```bash
# Check health status
curl http://localhost:9090/health

# View metrics
curl http://localhost:9090/metrics
```

## Troubleshooting

### Common Issues
1. **SSL Certificate Errors**: Verify certificate paths and permissions
2. **Authentication Failures**: Check user database and key files
3. **Performance Issues**: Monitor resource usage and adjust limits
4. **Script Execution Errors**: Verify script permissions and paths

### Debug Mode
Enable debug logging:
```ini
log_level = debug
log_console_output = true
```

## Customization

### Adding Modules
```ini
[module:custom]
path = /var/lib/simple-rsyncd/custom
comment = Custom module
read_only = false
list = true
delete = true
overwrite = true
exclude_patterns = *.tmp, *.log
include_patterns =
environment_variables = CUSTOM_MODE=production
```

### Performance Tuning
Adjust settings based on your environment:
- **Memory Usage**: Increase for high-memory systems
- **Buffer Size**: Larger buffers for high-bandwidth connections
- **Compression**: Adjust level based on CPU vs. bandwidth trade-offs
- **Concurrent Transfers**: Scale based on available resources
