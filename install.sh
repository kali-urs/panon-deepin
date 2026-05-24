#!/bin/bash
set -e

BUILD_DIR="build"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Building Panon for Deepin 25..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$PROJECT_DIR" -DCMAKE_INSTALL_PREFIX=/usr
cmake --build . -j$(nproc)

echo "Installing..."
sudo cmake --install .

echo "Restarting dock..."
killall dde-dock 2>/dev/null || true

echo "Done! Panon plugin installed to /usr/lib/dde-dock/plugins/"
echo "For development, you can also use:"
echo "  ln -sf $PROJECT_DIR/build/libpanon.so ~/.local/lib/dde-dock/plugins/libpanon.so"
