#!/bin/sh
# Cross-compiles the Windows .geode from Linux.
set -e
cd "$(dirname "$0")"
T="$HOME/.local/share/Geode/cross-tools"
export GEODE_SDK="${AUTODEAFEN_SDK:-$HOME/Documents/Geode-v6}"
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_TOOLCHAIN_FILE="$T/clang-msvc-sdk/clang-msvc.cmake" \
    -DSPLAT_DIR="$T/splat" -DHOST_ARCH=x64 -DGEODE_DONT_INSTALL_MODS=ON
cmake --build build-win
