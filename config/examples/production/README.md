# Production Configuration Examples

This directory contains production-ready configuration examples for simple-rsyncd designed for enterprise and cloud deployments.

## Configuration Files

### `enterprise.conf`
- **Purpose**: Full-featured enterprise production setup
- **Features**:
  - High connection limits (2000 max connections)
  - OAuth2 authentication
  - Comprehensive security
  - Advanced monitoring
  - Multiple specialized modules
- **Use Case**: Enterprise environments, large organizations
- **Modules**: `enterprise-documents`, `data-warehouse`, `backup-archive`, `software-distribution`, `media-library`, `compliance-logs`

### `cloud.conf`
- **Purpose**: Optimized for cloud deployments (AWS, Azure, GCP)
- **Features**:
  - Very high connection limits (5000 max connections)
  - Cloud-optimized performance
  - OAuth2 authentication
  - Comprehensive monitoring
  - Cloud-specific modules
- **Use Case**: Cloud deployments, CDN backends, large-scale services
- **Modules**: `cloud-storage`, `cdn-assets`, `data-pipeline`, `backup-sync`, `media-streaming`, `logs-aggregation`

## Prerequisites

### SSL/TLS Certificates
Production deployments require proper SSL certificates:
```bash
# Let's Encrypt (recommended)
certbot certonly --standalone -d your-domain.com

# Or commercial certificate
# Install certificate files to /etc/simple-rsyncd/ssl/
```

### Authentication Setup
OAuth2 configuration for enterprise/cloud deployments:
```json
{
    "client_id": "your-client-id",
    "client_secret": "your-client-secret",
    "token_endpoint": "https://your-auth-server/oauth/token",
    "userinfo_endpoint": "https://your-auth-server/oauth/userinfo",
    "scope": "rsync:read rsync:write"
}
```

### Directory Structure
Create comprehensive directory structure:
```bash
mkdir -p /var/lib/simple-rsyncd/{
    enterprise-documents,
    data-warehouse,
    backup-archive,
    software-distribution,
    media-library,
    compliance-logs
}
mkdir -p /var/log/simple-rsyncd
mkdir -p /etc/simple-rsyncd/ssl
chown -R rsync:rsync /var/lib/simple-rsyncd /var/log/simple-rsyncd
```

## Deployment

### Enterprise Deployment
1. **Copy configuration:**
   ```bash
   cp config/examples/production/enterprise.conf /etc/simple-rsyncd/rsyncd.conf
   ```

2. **Set up authentication:**
   ```bash
   # Configure OAuth2
   cp oauth2.json /etc/simple-rsyncd/oauth2.json

   # Set up user database
   htpasswd -c /etc/simple-rsyncd/users admin
   ```

3. **Install SSL certificates:**
   ```bash
   cp server.crt /etc/simple-rsyncd/ssl/
   cp server.key /etc/simple-rsyncd/ssl/
   cp ca.crt /etc/simple-rsyncd/ssl/
   chmod 600 /etc/simple-rsyncd/ssl/server.key
   ```

4. **Start the daemon:**
   ```bash
   systemctl start simple-rsyncd
   systemctl enable simple-rsyncd
   ```

### Cloud Deployment
1. **Copy configuration:**
   ```bash
   cp config/examples/production/cloud.conf /etc/simple-rsyncd/rsyncd.conf
   ```

2. **Configure cloud-specific settings:**
   ```bash
   # Set instance ID
   export INSTANCE_ID=$(curl -s http://169.254.169.254/latest/meta-data/instance-id)

   # Set region
   export AWS_REGION=$(curl -s http://169.254.169.254/latest/meta-data/placement/region)
   ```

3. **Deploy with container orchestration:**
   ```bash
   # Docker
   docker run -d --name simple-rsyncd \
     -p 873:873 \
     -v /etc/simple-rsyncd:/etc/simple-rsyncd:ro \
     -v /var/lib/simple-rsyncd:/var/lib/simple-rsyncd \
     simple-rsyncd:latest

   # Kubernetes
   kubectl apply -f k8s-deployment.yaml
   ```

## Performance Optimization

### Enterprise Settings
- **Memory**: 4GB maximum usage
- **Connections**: 2000 max connections
- **Workers**: 32 worker threads
- **Buffer**: 512KB buffer size
- **Compression**: Level 6 for balanced performance

### Cloud Settings
- **Memory**: 8GB maximum usage
- **Connections**: 5000 max connections
- **Workers**: 64 worker threads
- **Buffer**: 1MB buffer size
- **Compression**: Level 6 for cloud optimization

## Monitoring and Alerting

### Prometheus Integration
```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'simple-rsyncd'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: '/metrics'
    scrape_interval: 15s
```

### Grafana Dashboard
Import dashboard for comprehensive monitoring:
- Connection metrics
- Transfer rates
- Error rates
- Resource usage
- Performance metrics

### Alerting Rules
```yaml
# alerting.yml
groups:
  - name: simple-rsyncd
    rules:
      - alert: HighConnectionCount
        expr: simple_rsyncd_active_connections > 1000
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High connection count detected"
```

## Security Considerations

### Network Security
- Use firewalls to restrict access
- Implement VPN for remote access
- Enable DDoS protection
- Use load balancers with SSL termination

### Access Control
- Implement OAuth2 for authentication
- Use role-based access control
- Enable audit logging
- Regular security audits

### Data Protection
- Encrypt data at rest
- Use SSL/TLS for data in transit
- Implement backup encryption
- Regular security updates

## Backup and Recovery

### Backup Strategy
```bash
#!/bin/bash
# backup.sh
DATE=$(date +%Y%m%d_%H%M%S)

# Backup configuration
tar -czf "rsyncd-config-${DATE}.tar.gz" /etc/simple-rsyncd/

# Backup data
tar -czf "rsyncd-data-${DATE}.tar.gz" /var/lib/simple-rsyncd/

# Upload to cloud storage
aws s3 cp "rsyncd-config-${DATE}.tar.gz" s3://your-backup-bucket/
aws s3 cp "rsyncd-data-${DATE}.tar.gz" s3://your-backup-bucket/
```

### Disaster Recovery
- Regular backup testing
- Cross-region replication
- Automated failover procedures
- Recovery time objectives (RTO)
- Recovery point objectives (RPO)

## Scaling

### Horizontal Scaling
- Load balancer configuration
- Multiple instance deployment
- Session affinity considerations
- Health check configuration

### Vertical Scaling
- Resource monitoring
- Performance tuning
- Capacity planning
- Auto-scaling policies

## Maintenance

### Regular Tasks
- Security updates
- Performance monitoring
- Log rotation
- Certificate renewal
- Backup verification

### Monitoring
- Health checks
- Performance metrics
- Error rates
- Resource usage
- Security events
