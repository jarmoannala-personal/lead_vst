#!/bin/bash
# Build and run LeadSynth standalone app
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
APP="$BUILD_DIR/LeadSynth_artefacts/Release/Standalone/LeadSynth.app"

# Configure if needed
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release "$PROJECT_DIR"
fi

cmake --build "$BUILD_DIR" --config Release --target LeadSynth_Standalone

echo "Build complete: $APP"

if [ "$1" = "--run" ]; then
    open "$APP"
fi
