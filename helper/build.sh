#!/bin/sh
# Builds the Linux helper. Static so it needs no system libraries, and PIE
# because Proton blocks system calls from low addresses.
set -e
cd "$(dirname "$0")/.."
mkdir -p build-helper
g++ -std=c++23 -O2 -Wall -Wextra -Werror -static-pie -fPIE -fno-exceptions -fno-rtti -s \
    -o build-helper/autodeafen-helper helper/main.cpp
