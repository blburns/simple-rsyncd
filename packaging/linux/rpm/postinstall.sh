#!/bin/bash
# Post-installation script for simple-rsyncd RPM

set -e

PROJECT_NAME="simple-rsyncd"
SERVICE_USER="rsyncddev"

# Create service user if it doesn't exist
if ! id "$SERVICE_USER" &>/dev/null; then
    useradd -r -s /sbin/nologin -d /var/lib/$simple-rsyncd -c "$simple-rsyncd service user" "$SERVICE_USER"
fi

# Set ownership
chown -R "$SERVICE_USER:$SERVICE_USER" /etc/$simple-rsyncd 2>/dev/null || true
chown -R "$SERVICE_USER:$SERVICE_USER" /var/log/$simple-rsyncd 2>/dev/null || true
chown -R "$SERVICE_USER:$SERVICE_USER" /var/lib/$simple-rsyncd 2>/dev/null || true

# Enable and start service
systemctl daemon-reload
systemctl enable "$simple-rsyncd" 2>/dev/null || true
systemctl start "$simple-rsyncd" 2>/dev/null || true

exit 0

