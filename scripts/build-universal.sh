#!/bin/bash
# Build LeadSynth as universal binary (arm64 + x86_64) for distribution
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build-universal"

echo "=== LeadSynth Universal Build (arm64 + x86_64) ==="

# Always reconfigure to ensure architectures are set correctly
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    CACHED_ARCHS=$(grep CMAKE_OSX_ARCHITECTURES:STRING= "$BUILD_DIR/CMakeCache.txt" | cut -d= -f2)
    if [ "$CACHED_ARCHS" != "arm64;x86_64" ]; then
        echo "Architecture mismatch in cache; cleaning..."
        rm -rf "$BUILD_DIR"
    fi
fi

if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "Configuring CMake for universal build..."
    cmake -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
        "$PROJECT_DIR"
fi

echo "Building universal binaries..."
cmake --build "$BUILD_DIR" --config Release

VST3="$BUILD_DIR/LeadSynth_artefacts/Release/VST3/LeadSynth.vst3"
AU="$BUILD_DIR/LeadSynth_artefacts/Release/AU/LeadSynth.component"

echo ""
echo "Verifying architectures:"
for binary in \
    "$VST3/Contents/MacOS/LeadSynth" \
    "$AU/Contents/MacOS/LeadSynth"; do
    if [ -f "$binary" ]; then
        echo "  $(basename "$(dirname "$(dirname "$binary")")"): $(lipo -archs "$binary")"
    fi
done

echo ""
echo "Build complete:"
echo "  VST3: $VST3"
echo "  AU:   $AU"
