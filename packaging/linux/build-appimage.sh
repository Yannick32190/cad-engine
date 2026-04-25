#!/bin/bash
# ===========================================================================
# CAD-ENGINE — Linux AppImage Builder
# ===========================================================================
# Usage:  ./build-appimage.sh [--clean]
#
# Prerequisites (Ubuntu/Debian/Mint):
#   sudo apt install build-essential cmake git
#   sudo apt install qt6-base-dev qt6-base-dev-tools libqt6opengl6-dev
#   sudo apt install libgl1-mesa-dev libglu1-mesa-dev
#   + OpenCASCADE 7.9.x compiled from source (see BUILD.md)
#
# Output: CAD-ENGINE-v0.8.0-x86_64.AppImage
# ===========================================================================

set -e

VERSION="1.0.0"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-release"
APPDIR="$PROJECT_DIR/build-release/AppDir"

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${CYAN}[INFO]${NC} $1"; }
ok()    { echo -e "${GREEN}[OK]${NC} $1"; }
fail()  { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }

# ===========================================================================
# Clean
# ===========================================================================
if [ "$1" = "--clean" ]; then
    info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# ===========================================================================
# Step 1: Build
# ===========================================================================
info "Building CAD-ENGINE v${VERSION} (Release)..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTS=OFF \
    -DPORTABLE_BUILD=ON

make -j$(nproc)

ok "Build complete"

# ===========================================================================
# Step 2: Create AppDir structure
# ===========================================================================
info "Creating AppDir..."

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/plugins/platforms"
mkdir -p "$APPDIR/usr/plugins/xcbglintegrations"
mkdir -p "$APPDIR/usr/plugins/opengl"

# Copy binary
cp "$BUILD_DIR/cad-engine" "$APPDIR/usr/bin/"
strip "$APPDIR/usr/bin/cad-engine"

# Copy desktop + icon
cp "$SCRIPT_DIR/cad-engine.desktop" "$APPDIR/"
cp "$SCRIPT_DIR/cad-engine.desktop" "$APPDIR/usr/share/applications/"
cp "$SCRIPT_DIR/cad-engine.png" "$APPDIR/"
cp "$SCRIPT_DIR/cad-engine.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/"

ok "AppDir structure created"

# ===========================================================================
# Step 3: Bundle OpenCASCADE libraries
# ===========================================================================
info "Bundling OpenCASCADE libraries..."

OCCT_LIB_DIR="/usr/local/lib"
if [ ! -d "$OCCT_LIB_DIR" ]; then
    OCCT_LIB_DIR="/usr/lib/x86_64-linux-gnu"
fi

OCCT_LIBS=(
    TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep
    TKGeomAlgo TKTopAlgo TKPrim TKShHealing TKMesh
    TKBool TKBO TKFillet TKOffset TKFeat
    TKBin TKBinL TKLCAF TKCAF TKCDF
)

occt_count=0
for lib in "${OCCT_LIBS[@]}"; do
    src="$OCCT_LIB_DIR/lib${lib}.so"
    if [ -f "$src" ]; then
        # Copy the actual file, not the symlink
        real=$(readlink -f "$src")
        cp "$real" "$APPDIR/usr/lib/"
        # Create symlinks
        basename_real=$(basename "$real")
        ln -sf "$basename_real" "$APPDIR/usr/lib/lib${lib}.so"
        # Also create .so.7 symlink if needed
        if [[ "$basename_real" == *".so."* ]]; then
            major_so="lib${lib}.so.$(echo $basename_real | grep -oP '\.so\.\K[0-9]+')"
            ln -sf "$basename_real" "$APPDIR/usr/lib/$major_so" 2>/dev/null || true
        fi
        ((occt_count++))
    fi
done

# Also copy OCCT resource files if they exist
if [ -d "/usr/local/share/opencascade" ]; then
    mkdir -p "$APPDIR/usr/share/opencascade"
    cp -r /usr/local/share/opencascade/* "$APPDIR/usr/share/opencascade/" 2>/dev/null || true
fi

ok "Bundled $occt_count OCCT libraries"

# ===========================================================================
# Step 4: Bundle Qt6 libraries
# ===========================================================================
info "Bundling Qt6 libraries..."

# Find Qt6 lib directory
QT6_DIR=$(qmake6 -query QT_INSTALL_LIBS 2>/dev/null || qtpaths6 --install-prefix 2>/dev/null)
if [ -z "$QT6_DIR" ]; then
    # Fallback
    QT6_DIR=$(find /usr/lib -name "libQt6Core.so*" -printf '%h\n' 2>/dev/null | head -1)
fi

QT6_PLUGIN_DIR=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || echo "/usr/lib/x86_64-linux-gnu/qt6/plugins")

QT6_LIBS=(
    libQt6Core.so
    libQt6Gui.so
    libQt6Widgets.so
    libQt6OpenGL.so
    libQt6OpenGLWidgets.so
    libQt6DBus.so
    libQt6XcbQpa.so
    libQt6WaylandClient.so
    libQt6EglFSDeviceIntegration.so
)

qt_count=0
for lib in "${QT6_LIBS[@]}"; do
    src=$(find "$QT6_DIR" /usr/lib -name "${lib}*" -not -type d 2>/dev/null | head -1)
    if [ -n "$src" ] && [ -f "$src" ]; then
        real=$(readlink -f "$src")
        cp "$real" "$APPDIR/usr/lib/"
        basename_real=$(basename "$real")
        base="${lib%.so}"
        ln -sf "$basename_real" "$APPDIR/usr/lib/${lib}" 2>/dev/null || true
        ((qt_count++))
    fi
done

# Qt plugins
for plugin_dir in platforms xcbglintegrations wayland-shell-integration; do
    plugin_src="$QT6_PLUGIN_DIR/$plugin_dir"
    if [ -d "$plugin_src" ]; then
        mkdir -p "$APPDIR/usr/plugins/$plugin_dir"
        cp -L "$plugin_src"/*.so "$APPDIR/usr/plugins/$plugin_dir/" 2>/dev/null || true
    fi
done

ok "Bundled $qt_count Qt6 libraries + plugins"

# ===========================================================================
# Step 5: Bundle system dependencies
# ===========================================================================
info "Bundling system dependencies..."

# Use ldd to find remaining deps
sys_count=0
for dep in $(ldd "$APPDIR/usr/bin/cad-engine" | grep "=> /" | awk '{print $3}'); do
    basename_dep=$(basename "$dep")
    # Skip basic system libs that AppImage should NOT bundle
    case "$basename_dep" in
        libc.so*|libm.so*|libdl.so*|libpthread.so*|librt.so*|ld-linux*|libGL.so*|\
        libGLX.so*|libGLdispatch*|libX11.so*|libxcb.so*|libdrm.so*|libnvidia*|\
        libstdc++.so*|libgcc_s.so*)
            continue ;;
    esac
    
    if [ ! -f "$APPDIR/usr/lib/$basename_dep" ]; then
        cp -L "$dep" "$APPDIR/usr/lib/" 2>/dev/null && ((sys_count++)) || true
    fi
done

ok "Bundled $sys_count additional system libraries"

# ===========================================================================
# Step 6: Create AppRun
# ===========================================================================
info "Creating AppRun..."

cat > "$APPDIR/AppRun" << 'APPRUN_EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}

# Library paths
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"

# Qt plugins
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms"

# OCCT resources
export CASROOT="${HERE}/usr/share/opencascade"
export CSF_OCCTResourcePath="${HERE}/usr/share/opencascade/resources"

# Wayland workaround — fallback to X11 if issues
if [ -z "$QT_QPA_PLATFORM" ]; then
    if [ "$XDG_SESSION_TYPE" = "wayland" ]; then
        export QT_QPA_PLATFORM=xcb
    fi
fi

exec "${HERE}/usr/bin/cad-engine" "$@"
APPRUN_EOF

chmod +x "$APPDIR/AppRun"

ok "AppRun created"

# ===========================================================================
# Step 7: Build AppImage
# ===========================================================================
info "Building AppImage..."

APPIMAGETOOL="$BUILD_DIR/appimagetool"
if [ ! -f "$APPIMAGETOOL" ]; then
    info "Downloading appimagetool..."
    ARCH=$(uname -m)
    wget -q -O "$APPIMAGETOOL" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${ARCH}.AppImage" \
        || wget -q -O "$APPIMAGETOOL" \
        "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-${ARCH}.AppImage"
    chmod +x "$APPIMAGETOOL"
fi

OUTPUT="$PROJECT_DIR/CAD-ENGINE-v${VERSION}-x86_64.AppImage"

# Try running appimagetool (FUSE or extract)
if "$APPIMAGETOOL" --appimage-extract-and-run "$APPDIR" "$OUTPUT" 2>/dev/null; then
    ok "AppImage created: $OUTPUT"
elif "$APPIMAGETOOL" "$APPDIR" "$OUTPUT" 2>/dev/null; then
    ok "AppImage created: $OUTPUT"
else
    # Fallback: create a self-extracting tar
    info "appimagetool unavailable, creating portable tar.gz..."
    OUTPUT="$PROJECT_DIR/CAD-ENGINE-v${VERSION}-linux-x86_64.tar.gz"
    cd "$APPDIR/.."
    tar czf "$OUTPUT" -C "$APPDIR/.." AppDir --transform="s/AppDir/CAD-ENGINE-v${VERSION}/"
    ok "Portable archive created: $OUTPUT"
    info "To run: extract and execute ./CAD-ENGINE-v${VERSION}/AppRun"
fi

# ===========================================================================
# Summary
# ===========================================================================
echo ""
echo "============================================"
echo "  CAD-ENGINE v${VERSION} — Package Ready"
echo "============================================"
echo "  Output: $OUTPUT"
echo "  Size:   $(du -h "$OUTPUT" | cut -f1)"
echo "============================================"
