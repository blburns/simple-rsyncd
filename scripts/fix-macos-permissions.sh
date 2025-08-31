#!/bin/bash
# Fix macOS permissions for Simple RSync Daemon installation
# This script fixes common permission issues that prevent installation

set -e

echo "🔧 Fixing macOS permissions for Simple RSync Daemon installation..."

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ This script is for macOS only"
    exit 1
fi

# Check if running as root
if [[ $EUID -eq 0 ]]; then
    echo "❌ This script should not be run as root"
    exit 1
fi

echo "📋 Current user: $(whoami)"
echo "📋 Current group: $(id -gn)"

# Check current ownership of /usr/local
echo "🔍 Checking /usr/local ownership..."
ls -ld /usr/local

# Fix ownership of /usr/local directories
echo "🔧 Fixing ownership of /usr/local directories..."
sudo chown -R $(whoami):admin /usr/local/include
sudo chown -R $(whoami):admin /usr/local/lib
sudo chown -R $(whoami):admin /usr/local/bin
sudo chown -R $(whoami):admin /usr/local/etc
sudo chown -R $(whoami):admin /usr/local/share

# Fix permissions
echo "🔧 Fixing permissions..."
sudo chmod 755 /usr/local
sudo chmod 755 /usr/local/include
sudo chmod 755 /usr/local/lib
sudo chmod 755 /usr/local/bin
sudo chmod 755 /usr/local/etc
sudo chmod 755 /usr/local/share

# Verify changes
echo "✅ Verifying changes..."
ls -ld /usr/local
ls -ld /usr/local/include
ls -ld /usr/local/lib
ls -ld /usr/local/bin
ls -ld /usr/local/etc
ls -ld /usr/local/share

echo ""
echo "🎉 Permission fixes completed!"
echo ""
echo "You can now try installing Simple RSync Daemon again:"
echo "  cd build"
echo "  make install"
echo ""
echo "If you still encounter issues, you can also try:"
echo "  cmake .. -DCMAKE_INSTALL_PREFIX=\$HOME/.local"
echo "  make install"
echo ""
echo "This will install to your home directory instead of /usr/local"
