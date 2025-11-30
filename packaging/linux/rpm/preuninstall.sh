#!/bin/bash
# Pre-uninstallation script for simple-rsyncd RPM

set -e

PROJECT_NAME="simple-rsyncd"

# Stop service before removal
if [ "$1" -eq 0 ]; then
    systemctl stop "$simple-rsyncd" 2>/dev/null || true
    systemctl disable "$simple-rsyncd" 2>/dev/null || true
fi

exit 0

