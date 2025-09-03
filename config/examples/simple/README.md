# Simple Configuration Examples

This directory contains basic configuration examples for simple-rsyncd suitable for development, testing, and small-scale deployments.

## Configuration Files

### `basic.conf`
- **Purpose**: Minimal setup for development and testing
- **Features**: Basic security, moderate performance settings
- **Use Case**: Development environments, small teams
- **Modules**: `public` (read-only), `backup` (read-write)

### `development.conf`
- **Purpose**: Optimized for development environments
- **Features**: Debug logging, relaxed security, local-only access
- **Use Case**: Local development, testing, debugging
- **Modules**: `dev` (development workspace), `test` (test files)

### `read-only.conf`
- **Purpose**: Secure read-only file sharing
- **Features**: Enhanced security, chroot, read-only modules
- **Use Case**: Public file sharing, document distribution
- **Modules**: `public`, `documents`, `software`, `media` (all read-only)

## Quick Start

1. **Copy a configuration file:**
   ```bash
   cp config/examples/simple/basic.conf /etc/simple-rsyncd/rsyncd.conf
   ```

2. **Create necessary directories:**
   ```bash
   mkdir -p /var/lib/simple-rsyncd/{public,backup}
   mkdir -p /var/log/simple-rsyncd
   chown -R rsync:rsync /var/lib/simple-rsyncd /var/log/simple-rsyncd
   ```

3. **Start the daemon:**
   ```bash
   simple-rsyncd start --config /etc/simple-rsyncd/rsyncd.conf
   ```

## Configuration Notes

- **Security**: Basic access controls and rate limiting enabled
- **Performance**: Moderate settings suitable for small to medium workloads
- **Logging**: Console and file logging enabled
- **Monitoring**: Basic metrics collection enabled

## Customization

Edit the configuration file to:
- Change port numbers
- Modify access controls
- Adjust performance settings
- Add or remove modules
- Configure logging levels

## Testing

Test the configuration:
```bash
# Test configuration syntax
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

# Test rsync connection
rsync rsync://localhost/

# List available modules
rsync rsync://localhost/
```
