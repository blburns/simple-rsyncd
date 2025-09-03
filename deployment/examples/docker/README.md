# Simple RSync Daemon - Docker Deployment

This directory contains Docker deployment examples for simple-rsyncd.

## Quick Start

### Prerequisites

- Docker and Docker Compose installed
- Basic understanding of rsync protocol

### Basic Deployment

1. **Prepare the environment:**
   ```bash
   # Create necessary directories
   mkdir -p config data logs

   # Copy configuration template
   cp ../../../config/rsyncd.conf.example config/rsyncd.conf
   ```

2. **Start the service:**
   ```bash
   # Build and start the container
   docker-compose up -d

   # Check status
   docker-compose ps
   ```

3. **Test the connection:**
   ```bash
   # Test rsync connection
   rsync rsync://localhost/

   # List available modules
   rsync rsync://localhost/
   ```

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SIMPLE_RSYNCD_CONFIG` | `/etc/simple-rsyncd/rsyncd.conf` | Configuration file path |
| `SIMPLE_RSYNCD_LOG_LEVEL` | `info` | Log level (debug, info, warn, error) |
| `SIMPLE_RSYNCD_BIND_ADDRESS` | `0.0.0.0` | Bind address |
| `SIMPLE_RSYNCD_BIND_PORT` | `873` | Bind port |

### Volume Mounts

| Host Path | Container Path | Description |
|-----------|----------------|-------------|
| `./config` | `/etc/simple-rsyncd` | Configuration files (read-only) |
| `./data` | `/var/lib/simple-rsyncd` | Data storage |
| `./logs` | `/var/log/simple-rsyncd` | Log files |

### Configuration File

Edit `config/rsyncd.conf` to customize the daemon:

```ini
[global]
bind_address = 0.0.0.0
bind_port = 873
ssl_enabled = false
auth_enabled = false
log_level = info

[module:backup]
path = /var/lib/simple-rsyncd/backup
comment = Backup storage
read_only = false
list = true
delete = true
overwrite = true

[module:public]
path = /var/lib/simple-rsyncd/public
comment = Public files
read_only = true
list = true
delete = false
overwrite = false
```

## Production Deployment

### Security Considerations

1. **Enable SSL/TLS:**
   ```ini
   [global]
   ssl_enabled = true
   ssl_certificate_file = /etc/simple-rsyncd/ssl/server.crt
   ssl_private_key_file = /etc/simple-rsyncd/ssl/server.key
   ```

2. **Enable Authentication:**
   ```ini
   [global]
   auth_enabled = true
   auth_method = password
   auth_password_file = /etc/simple-rsyncd/users
   ```

3. **Restrict Access:**
   ```ini
   [global]
   access_enabled = true
   access_allowed_hosts = 192.168.1.0/24, 10.0.0.0/8
   ```

### Resource Limits

Add resource limits to docker-compose.yml:

```yaml
services:
  simple-rsyncd:
    # ... existing configuration ...
    deploy:
      resources:
        limits:
          cpus: '2.0'
          memory: 1G
        reservations:
          cpus: '0.5'
          memory: 256M
```

### Monitoring

Enable monitoring in configuration:

```ini
[global]
monitoring_enabled = true
monitoring_metrics_file = /var/lib/simple-rsyncd/metrics.json
monitoring_health_check_file = /var/lib/simple-rsyncd/health.json
monitoring_enable_prometheus_metrics = true
monitoring_prometheus_port = 9090
```

## Development Environment

### Using Development Profile

```bash
# Start development environment
docker-compose --profile dev up dev

# Access development container
docker-compose exec dev bash

# Build the application
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Debugging

```bash
# View logs
docker-compose logs -f simple-rsyncd

# Access container shell
docker-compose exec simple-rsyncd bash

# Check health status
docker inspect --format='{{.State.Health.Status}}' simple-rsyncd
```

## Client Usage

### Basic rsync Commands

```bash
# List available modules
rsync rsync://localhost/

# List module contents
rsync rsync://localhost/backup/

# Upload files
rsync -avz /local/path/ rsync://localhost/backup/

# Download files
rsync -avz rsync://localhost/backup/ /local/path/

# With authentication
rsync -avz --password-file=/path/to/password /local/path/ rsync://user@localhost/backup/
```

### SSL/TLS Usage

```bash
# With SSL/TLS (if enabled)
rsync -avz -e "rsync --rsh='openssl s_client -quiet -connect localhost:873'" /local/path/ /backup/
```

## Troubleshooting

### Common Issues

1. **Connection Refused:**
   ```bash
   # Check if container is running
   docker-compose ps

   # Check logs
   docker-compose logs simple-rsyncd

   # Test port connectivity
   nc -z localhost 873
   ```

2. **Permission Denied:**
   ```bash
   # Check file permissions
   ls -la data/

   # Fix ownership
   sudo chown -R 1000:1000 data/ logs/
   ```

3. **Configuration Errors:**
   ```bash
   # Test configuration
   docker-compose exec simple-rsyncd simple-rsyncd --test --config /etc/simple-rsyncd/rsyncd.conf
   ```

### Log Analysis

```bash
# View real-time logs
docker-compose logs -f simple-rsyncd

# View specific log files
docker-compose exec simple-rsyncd tail -f /var/log/simple-rsyncd/rsyncd.log

# Check access logs
docker-compose exec simple-rsyncd tail -f /var/log/simple-rsyncd/access.log
```

### Performance Tuning

1. **Buffer Size:**
   ```ini
   [global]
   performance_buffer_size = 128KB
   ```

2. **Compression:**
   ```ini
   [global]
   performance_enable_compression = true
   performance_compression_level = 6
   ```

3. **Concurrent Transfers:**
   ```ini
   [global]
   performance_max_concurrent_transfers = 20
   ```

## Backup and Recovery

### Data Backup

```bash
# Backup data directory
tar -czf rsyncd-backup-$(date +%Y%m%d).tar.gz data/

# Backup configuration
cp -r config/ rsyncd-config-backup/
```

### Container Backup

```bash
# Export container
docker export simple-rsyncd > simple-rsyncd-backup.tar

# Import container
docker import simple-rsyncd-backup.tar simple-rsyncd:backup
```

## Scaling

### Load Balancing

For high-traffic deployments, consider:

1. **Multiple Instances:**
   ```yaml
   services:
     simple-rsyncd-1:
       # ... configuration ...
       ports:
         - "873:873"

     simple-rsyncd-2:
       # ... configuration ...
       ports:
         - "874:873"
   ```

2. **External Load Balancer:**
   - Use nginx, haproxy, or cloud load balancer
   - Configure health checks
   - Implement session affinity if needed

## Integration

### Kubernetes

See the Kubernetes deployment examples in the parent directory.

### CI/CD

```yaml
# .github/workflows/docker.yml
name: Docker Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build Docker image
        run: docker-compose build
      - name: Test Docker image
        run: docker-compose up -d && docker-compose ps
```

## Support

- **Documentation**: [../../docs/](../docs/)
- **Issues**: [GitHub Issues](https://github.com/simple-rsyncd/simple-rsyncd/issues)
- **Discussions**: [GitHub Discussions](https://github.com/simple-rsyncd/simple-rsyncd/discussions)

---

**Simple RSync Daemon** - Secure, fast, and reliable file synchronization with Docker.
