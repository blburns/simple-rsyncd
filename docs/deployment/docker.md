# Docker Deployment Guide

This guide provides comprehensive instructions for deploying simple-rsyncd using Docker and Docker Compose.

## Table of Contents

- [Quick Start](#quick-start)
- [Docker Images](#docker-images)
- [Configuration](#configuration)
- [Deployment Profiles](#deployment-profiles)
- [Multi-Platform Builds](#multi-platform-builds)
- [Production Deployment](#production-deployment)
- [Development Environment](#development-environment)
- [Monitoring and Health Checks](#monitoring-and-health-checks)
- [Security Considerations](#security-considerations)
- [Performance Tuning](#performance-tuning)
- [Troubleshooting](#troubleshooting)
- [Advanced Usage](#advanced-usage)

## Quick Start

### Prerequisites

- Docker 20.10+ and Docker Compose 2.0+
- Basic understanding of rsync protocol
- 2GB+ available disk space

### Basic Deployment

1. **Clone and navigate to the project:**
   ```bash
   git clone https://github.com/simple-rsyncd/simple-rsyncd.git
   cd simple-rsyncd
   ```

2. **Quick deployment:**
   ```bash
   # Navigate to Docker examples
   cd deployment/examples/docker

   # Create configuration directory
   mkdir -p config data logs
   cp ../../../config/rsyncd.conf.example config/rsyncd.conf

   # Start the service
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

## Docker Images

### Available Images

The project provides several Docker images for different use cases:

| Image | Purpose | Size | Use Case |
|-------|---------|------|----------|
| `simple-rsyncd:latest` | Runtime | ~50MB | Production deployment |
| `simple-rsyncd:dev` | Development | ~500MB | Development with tools |
| `simple-rsyncd:ubuntu` | Ubuntu build | ~200MB | Ubuntu-specific builds |
| `simple-rsyncd:centos` | CentOS build | ~200MB | CentOS-specific builds |
| `simple-rsyncd:alpine` | Alpine build | ~100MB | Minimal Alpine builds |

### Multi-Architecture Support

The Docker images support multiple architectures:

- **linux/amd64** (x86_64)
- **linux/arm64** (ARM64)
- **linux/arm/v7** (ARMv7)

### Image Layers

```
simple-rsyncd:latest
├── ubuntu:22.04 (base)
├── runtime dependencies (libssl3, libjsoncpp25)
├── simple-rsyncd binary
├── configuration files
└── non-root user setup
```

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SIMPLE_RSYNCD_CONFIG` | `/etc/simple-rsyncd/rsyncd.conf` | Configuration file path |
| `SIMPLE_RSYNCD_LOG_LEVEL` | `info` | Log level (debug, info, warn, error) |
| `SIMPLE_RSYNCD_BIND_ADDRESS` | `0.0.0.0` | Bind address |
| `SIMPLE_RSYNCD_BIND_PORT` | `873` | Bind port |
| `SIMPLE_RSYNCD_SSL_ENABLED` | `false` | Enable SSL/TLS |
| `SIMPLE_RSYNCD_AUTH_ENABLED` | `false` | Enable authentication |

### Volume Mounts

| Host Path | Container Path | Description | Mode |
|-----------|----------------|-------------|------|
| `./config` | `/etc/simple-rsyncd` | Configuration files | ro |
| `./data` | `/var/lib/simple-rsyncd` | Data storage | rw |
| `./logs` | `/var/log/simple-rsyncd` | Log files | rw |
| `./ssl` | `/etc/simple-rsyncd/ssl` | SSL certificates | ro |

### Configuration File

The main configuration file is mounted from the host:

```ini
# /etc/simple-rsyncd/rsyncd.conf
[global]
bind_address = 0.0.0.0
bind_port = 873
ssl_enabled = false
auth_enabled = false
log_level = info
max_connections = 100
worker_threads = 4

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

## Deployment Profiles

### Runtime Profile

For production deployments:

```bash
# Deploy runtime environment
docker-compose --profile runtime up -d simple-rsyncd

# Or using the deployment script
./scripts/deploy-docker.sh -p runtime
```

**Features:**
- Optimized for production
- Minimal image size
- Health checks enabled
- Automatic restart
- Resource limits

### Development Profile

For development and debugging:

```bash
# Deploy development environment
docker-compose --profile dev up -d dev

# Or using the deployment script
./scripts/deploy-docker.sh -p dev
```

**Features:**
- Development tools included
- Source code mounted
- Debug symbols
- Interactive shell access
- Hot reloading

### Build Profile

For building images:

```bash
# Build for specific distribution
docker-compose --profile build build build-ubuntu
docker-compose --profile build build build-centos
docker-compose --profile build build build-alpine

# Or using the build script
./scripts/build-docker.sh -d ubuntu
```

## Multi-Platform Builds

### Using Build Script

```bash
# Build for all distributions
./scripts/build-docker.sh -d all

# Build for all architectures
./scripts/build-docker.sh -a all

# Build multi-architecture image
./scripts/build-docker.sh -d all -a all

# Build with custom registry
./scripts/build-docker.sh -r myregistry.com -t v1.0.0

# Build and push
./scripts/build-docker.sh --push -r myregistry.com
```

### Manual Multi-Architecture Build

```bash
# Create buildx builder
docker buildx create --name rsyncd-builder --use

# Build multi-architecture image
docker buildx build \
  --platform linux/amd64,linux/arm64,linux/arm/v7 \
  --tag simple-rsyncd:latest \
  --push \
  .
```

### Build Arguments

| Argument | Description | Default |
|----------|-------------|---------|
| `PLATFORM` | Target platform | `linux` |
| `DISTRO` | Target distribution | `ubuntu` |
| `ARCH` | Target architecture | `x86_64` |
| `TARGETPLATFORM` | Buildx target platform | Auto-detected |

## Production Deployment

### Security Configuration

1. **Enable SSL/TLS:**
   ```ini
   [global]
   ssl_enabled = true
   ssl_certificate_file = /etc/simple-rsyncd/ssl/server.crt
   ssl_private_key_file = /etc/simple-rsyncd/ssl/server.key
   ssl_ca_file = /etc/simple-rsyncd/ssl/ca.crt
   ssl_tls_version = 1.3
   ```

2. **Enable Authentication:**
   ```ini
   [global]
   auth_enabled = true
   auth_method = password
   auth_password_file = /etc/simple-rsyncd/users
   auth_realm = simple-rsyncd
   ```

3. **Restrict Access:**
   ```ini
   [global]
   access_enabled = true
   access_allowed_hosts = 192.168.1.0/24, 10.0.0.0/8
   access_denied_hosts = 192.168.1.100
   ```

### Resource Limits

```yaml
# docker-compose.yml
services:
  simple-rsyncd:
    deploy:
      resources:
        limits:
          cpus: '2.0'
          memory: 1G
        reservations:
          cpus: '0.5'
          memory: 256M
      restart_policy:
        condition: on-failure
        delay: 5s
        max_attempts: 3
```

### Health Checks

```yaml
# docker-compose.yml
services:
  simple-rsyncd:
    healthcheck:
      test: ["CMD", "nc", "-z", "localhost", "873"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 40s
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

### Backup Strategy

```bash
#!/bin/bash
# backup.sh

# Backup data
tar -czf "rsyncd-backup-$(date +%Y%m%d).tar.gz" data/

# Backup configuration
cp -r config/ "rsyncd-config-backup-$(date +%Y%m%d)/"

# Backup container
docker save simple-rsyncd:latest | gzip > "rsyncd-image-$(date +%Y%m%d).tar.gz"
```

## Development Environment

### Development Setup

```bash
# Start development environment
docker-compose --profile dev up -d dev

# Access development container
docker-compose exec dev bash

# Build the application
cd /app
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run tests
make test
```

### Debugging

```bash
# View logs
docker-compose logs -f dev

# Access container with debugger
docker-compose exec dev gdb /app/build/bin/simple-rsyncd

# Profile with valgrind
docker-compose exec dev valgrind --tool=memcheck /app/build/bin/simple-rsyncd
```

### Code Quality

```bash
# Format code
docker-compose exec dev clang-format -i src/**/*.cpp include/**/*.hpp

# Static analysis
docker-compose exec dev cppcheck --enable=all src/

# Security scan
docker-compose exec dev bandit -r src/
```

## Monitoring and Health Checks

### Health Check Endpoints

| Endpoint | Description | Response |
|----------|-------------|----------|
| `/health` | Basic health check | `{"status": "healthy"}` |
| `/metrics` | Prometheus metrics | Prometheus format |
| `/status` | Detailed status | JSON status object |

### Prometheus Integration

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'simple-rsyncd'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: '/metrics'
    scrape_interval: 30s
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "Simple RSync Daemon",
    "panels": [
      {
        "title": "Active Connections",
        "type": "stat",
        "targets": [
          {
            "expr": "simple_rsyncd_active_connections"
          }
        ]
      },
      {
        "title": "Transfer Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(simple_rsyncd_bytes_transferred[5m])"
          }
        ]
      }
    ]
  }
}
```

### Log Monitoring

```bash
# View real-time logs
docker-compose logs -f simple-rsyncd

# Filter logs by level
docker-compose logs -f simple-rsyncd | grep ERROR

# Export logs
docker-compose logs simple-rsyncd > rsyncd.log
```

## Security Considerations

### Container Security

1. **Non-root User:**
   ```dockerfile
   USER simple-rsyncd
   ```

2. **Read-only Root Filesystem:**
   ```yaml
   services:
     simple-rsyncd:
       read_only: true
       tmpfs:
         - /tmp
         - /var/run
   ```

3. **Security Context:**
   ```yaml
   services:
     simple-rsyncd:
       security_opt:
         - no-new-privileges:true
       cap_drop:
         - ALL
       cap_add:
         - NET_BIND_SERVICE
   ```

### Network Security

1. **Internal Networks:**
   ```yaml
   networks:
     rsync-internal:
       driver: bridge
       internal: true
   ```

2. **Firewall Rules:**
   ```bash
   # Allow only rsync port
   iptables -A INPUT -p tcp --dport 873 -j ACCEPT
   iptables -A INPUT -p tcp --dport 873 -j DROP
   ```

### SSL/TLS Configuration

```ini
[global]
ssl_enabled = true
ssl_certificate_file = /etc/simple-rsyncd/ssl/server.crt
ssl_private_key_file = /etc/simple-rsyncd/ssl/server.key
ssl_ca_file = /etc/simple-rsyncd/ssl/ca.crt
ssl_cipher_suite = ECDHE-RSA-AES256-GCM-SHA384
ssl_tls_version = 1.3
ssl_require_client_cert = true
```

## Performance Tuning

### Container Resources

```yaml
services:
  simple-rsyncd:
    deploy:
      resources:
        limits:
          cpus: '4.0'
          memory: 2G
        reservations:
          cpus: '1.0'
          memory: 512M
```

### Application Tuning

```ini
[global]
performance_buffer_size = 128KB
performance_max_memory_usage = 1GB
performance_enable_compression = true
performance_compression_level = 6
performance_max_concurrent_transfers = 20
performance_enable_pipelining = true
performance_pipeline_depth = 10
```

### Storage Optimization

```yaml
services:
  simple-rsyncd:
    volumes:
      - type: tmpfs
        target: /tmp
        tmpfs:
          size: 1G
      - type: bind
        source: /fast-storage
        target: /var/lib/simple-rsyncd
        bind:
          propagation: rshared
```

## Troubleshooting

### Common Issues

1. **Container Won't Start:**
   ```bash
   # Check logs
   docker-compose logs simple-rsyncd

   # Check configuration
   docker-compose exec simple-rsyncd simple-rsyncd --test --config /etc/simple-rsyncd/rsyncd.conf
   ```

2. **Permission Denied:**
   ```bash
   # Check file permissions
   ls -la data/ logs/

   # Fix ownership
   sudo chown -R 1000:1000 data/ logs/
   ```

3. **Connection Refused:**
   ```bash
   # Check if container is running
   docker-compose ps

   # Test port connectivity
   nc -z localhost 873

   # Check firewall
   iptables -L
   ```

4. **SSL/TLS Issues:**
   ```bash
   # Check certificate validity
   openssl x509 -in config/ssl/server.crt -text -noout

   # Test SSL connection
   openssl s_client -connect localhost:873
   ```

### Debug Mode

```bash
# Enable debug logging
export SIMPLE_RSYNCD_LOG_LEVEL=debug

# Start with debug output
docker-compose up simple-rsyncd
```

### Performance Issues

```bash
# Monitor resource usage
docker stats simple-rsyncd

# Check disk I/O
docker exec simple-rsyncd iostat -x 1

# Monitor network
docker exec simple-rsyncd netstat -i
```

## Advanced Usage

### Custom Builds

```dockerfile
# Custom Dockerfile
FROM simple-rsyncd:latest

# Add custom modules
COPY custom-modules/ /etc/simple-rsyncd/modules.d/

# Add custom scripts
COPY scripts/ /usr/local/bin/
RUN chmod +x /usr/local/bin/*.sh

# Custom configuration
COPY custom.conf /etc/simple-rsyncd/custom.conf
```

### Kubernetes Integration

```yaml
# k8s-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: simple-rsyncd
spec:
  replicas: 3
  selector:
    matchLabels:
      app: simple-rsyncd
  template:
    metadata:
      labels:
        app: simple-rsyncd
    spec:
      containers:
      - name: simple-rsyncd
        image: simple-rsyncd:latest
        ports:
        - containerPort: 873
        env:
        - name: SIMPLE_RSYNCD_CONFIG
          value: /etc/simple-rsyncd/rsyncd.conf
        volumeMounts:
        - name: config
          mountPath: /etc/simple-rsyncd
        - name: data
          mountPath: /var/lib/simple-rsyncd
      volumes:
      - name: config
        configMap:
          name: rsyncd-config
      - name: data
        persistentVolumeClaim:
          claimName: rsyncd-data
```

### CI/CD Integration

```yaml
# .github/workflows/docker.yml
name: Docker Build and Deploy
on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2

    - name: Build Docker image
      run: |
        docker build -t simple-rsyncd:${{ github.sha }} .
        docker tag simple-rsyncd:${{ github.sha }} simple-rsyncd:latest

    - name: Test Docker image
      run: |
        docker run -d --name test-rsyncd -p 873:873 simple-rsyncd:latest
        sleep 10
        nc -z localhost 873
        docker stop test-rsyncd
        docker rm test-rsyncd

    - name: Push to registry
      if: github.ref == 'refs/heads/main'
      run: |
        echo ${{ secrets.DOCKER_PASSWORD }} | docker login -u ${{ secrets.DOCKER_USERNAME }} --password-stdin
        docker push simple-rsyncd:${{ github.sha }}
        docker push simple-rsyncd:latest
```

### Load Balancing

```yaml
# docker-compose.yml
version: '3.8'
services:
  nginx:
    image: nginx:alpine
    ports:
      - "80:80"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf
    depends_on:
      - simple-rsyncd-1
      - simple-rsyncd-2
      - simple-rsyncd-3

  simple-rsyncd-1:
    image: simple-rsyncd:latest
    expose:
      - "873"
    networks:
      - rsync-internal

  simple-rsyncd-2:
    image: simple-rsyncd:latest
    expose:
      - "873"
    networks:
      - rsync-internal

  simple-rsyncd-3:
    image: simple-rsyncd:latest
    expose:
      - "873"
    networks:
      - rsync-internal

networks:
  rsync-internal:
    driver: bridge
    internal: true
```

## Support

- **Documentation**: [../README.md](../README.md)
- **Issues**: [GitHub Issues](https://github.com/simple-rsyncd/simple-rsyncd/issues)
- **Discussions**: [GitHub Discussions](https://github.com/simple-rsyncd/simple-rsyncd/discussions)
- **Email**: SimpleDaemons

---

**Simple RSync Daemon** - Secure, fast, and reliable file synchronization with Docker.
