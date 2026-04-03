#!/bin/bash
# Install LeadSynth plugins to system audio plugin folders
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
ARTEFACTS="$BUILD_DIR/LeadSynth_artefacts/Release"

VST3_SRC="$ARTEFACTS/VST3/LeadSynth.vst3"
AU_SRC="$ARTEFACTS/AU/LeadSynth.component"

VST3_DST="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DST="$HOME/Library/Audio/Plug-Ins/Components"

# Build first if artefacts don't exist
if [ ! -d "$VST3_SRC" ] || [ ! -d "$AU_SRC" ]; then
    echo "Plugins not built yet. Building..."
    "$SCRIPT_DIR/build-all.sh"
fi

echo "=== Installing LeadSynth Plugins ==="

# VST3
mkdir -p "$VST3_DST"
rm -rf "$VST3_DST/LeadSynth.vst3"
cp -R "$VST3_SRC" "$VST3_DST/"
echo "  VST3 installed: $VST3_DST/LeadSynth.vst3"

# AU
mkdir -p "$AU_DST"
rm -rf "$AU_DST/LeadSynth.component"
cp -R "$AU_SRC" "$AU_DST/"
echo "  AU installed:   $AU_DST/LeadSynth.component"

# Reset AU cache so DAWs pick up the new plugin
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo ""
echo "Done. Restart your DAW to load the updated plugins."
