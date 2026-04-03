#!/bin/bash
# Clean build directory
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "Removing build directory..."
rm -rf "$BUILD_DIR"
echo "Clean complete."
