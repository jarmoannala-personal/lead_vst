#!/bin/bash
# Build a macOS .pkg installer with universal VST3 + AU
# Installs to /Library/Audio/Plug-Ins (system-wide; requires admin during install)
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build-universal"
DIST_DIR="$PROJECT_DIR/dist"
STAGING="$DIST_DIR/staging"

VERSION=$(grep -E "project\(LeadSynth VERSION" "$PROJECT_DIR/CMakeLists.txt" | sed -E 's/.*VERSION ([0-9.]+).*/\1/')
IDENTIFIER="com.leadsynth.LeadSynth"
PKG_NAME="LeadSynth-${VERSION}-macOS-Universal.pkg"

VST3_SRC="$BUILD_DIR/LeadSynth_artefacts/Release/VST3/LeadSynth.vst3"
AU_SRC="$BUILD_DIR/LeadSynth_artefacts/Release/AU/LeadSynth.component"

if [ ! -d "$VST3_SRC" ] || [ ! -d "$AU_SRC" ]; then
    echo "Universal build not found. Run scripts/build-universal.sh first."
    exit 1
fi

echo "=== Building installer for LeadSynth ${VERSION} ==="

rm -rf "$STAGING"
mkdir -p "$STAGING/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGING/Library/Audio/Plug-Ins/Components"

echo "Staging plugins..."
cp -R "$VST3_SRC" "$STAGING/Library/Audio/Plug-Ins/VST3/"
cp -R "$AU_SRC"   "$STAGING/Library/Audio/Plug-Ins/Components/"

echo "Building component package..."
COMPONENT_PKG="$DIST_DIR/LeadSynth-component.pkg"
pkgbuild \
    --root "$STAGING" \
    --identifier "$IDENTIFIER" \
    --version "$VERSION" \
    --install-location "/" \
    "$COMPONENT_PKG"

echo "Building distribution package..."
DISTRIBUTION_XML="$DIST_DIR/distribution.xml"
cat > "$DISTRIBUTION_XML" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>LeadSynth ${VERSION}</title>
    <organization>com.leadsynth</organization>
    <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <volume-check>
        <allowed-os-versions>
            <os-version min="11.0"/>
        </allowed-os-versions>
    </volume-check>
    <choices-outline>
        <line choice="default">
            <line choice="${IDENTIFIER}"/>
        </line>
    </choices-outline>
    <choice id="default"/>
    <choice id="${IDENTIFIER}" visible="false">
        <pkg-ref id="${IDENTIFIER}"/>
    </choice>
    <pkg-ref id="${IDENTIFIER}" version="${VERSION}" onConclusion="none">LeadSynth-component.pkg</pkg-ref>
</installer-gui-script>
EOF

productbuild \
    --distribution "$DISTRIBUTION_XML" \
    --package-path "$DIST_DIR" \
    "$DIST_DIR/$PKG_NAME"

echo ""
echo "Installer created:"
echo "  $DIST_DIR/$PKG_NAME"
echo ""
echo "NOTE: This installer is unsigned. Users will need to right-click > Open"
echo "      to bypass Gatekeeper, or run:"
echo "        sudo xattr -dr com.apple.quarantine '$PKG_NAME'"
