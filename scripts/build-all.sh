#!/bin/bash
# Build all LeadSynth targets (Standalone, VST3, AU)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "=== LeadSynth Build ==="

# Configure if needed
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "Configuring CMake..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release "$PROJECT_DIR"
fi

echo "Building all targets..."
cmake --build "$BUILD_DIR" --config Release

echo ""
echo "Build complete:"
echo "  Standalone: $BUILD_DIR/LeadSynth_artefacts/Release/Standalone/LeadSynth.app"
echo "  VST3:       $BUILD_DIR/LeadSynth_artefacts/Release/VST3/LeadSynth.vst3"
echo "  AU:         $BUILD_DIR/LeadSynth_artefacts/Release/AU/LeadSynth.component"
