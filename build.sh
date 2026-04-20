#!/bin/bash
# ===========================================================================
# CAD-ENGINE — Quick Build (development)
# ===========================================================================
# Usage: ./build.sh [release|debug|clean]
# ===========================================================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"

case "${1:-release}" in
    clean)
        echo "[CLEAN] Removing build directory..."
        rm -rf "$PROJECT_DIR/build"
        echo "Done."
        exit 0
        ;;
    debug)
        BUILD_TYPE="Debug"
        ;;
    *)
        BUILD_TYPE="Release"
        ;;
esac

echo "========================================"
echo "  CAD-ENGINE — Build ($BUILD_TYPE)"
echo "========================================"

mkdir -p "$PROJECT_DIR/build"
cd "$PROJECT_DIR/build"

cmake "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_TESTS=OFF \
    2>&1 | tail -15

echo ""
echo "Compiling with $(nproc) threads..."
make -j$(nproc) 2>&1 | tail -5

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "  Build OK — ./build/cad-engine"
    echo "========================================"
    echo ""
    echo "  Run:  cd build && ./cad-engine"
    echo ""
fi
