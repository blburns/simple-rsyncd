#!/bin/bash
# Post-uninstallation script for simple-rsyncd RPM

set -e

# Reload systemd
systemctl daemon-reload

exit 0

