#!/bin/sh
# Manual check: says hello to the running Discord through every Wine given.
# Usage: proton_smoke.sh CLIENT_ID wine...   Each gets a fresh prefix.
cd "$(dirname "$0")/.."
id=$1; shift
mkdir -p build-tests
x86_64-w64-mingw32-g++ -std=c++23 -Wall -Wextra -static -mwindows -o build-tests/smoke.exe \
    tests/socket_smoke.cpp src/discord_socket.cpp src/helper_link.cpp src/private_folder.cpp -lws2_32 -ladvapi32 || exit 1
for wine in "$@"; do
    tmp=$(mktemp -d); mkdir "$tmp/package" "$tmp/save dir"
    chmod 755 "$tmp/save dir"
    # As left by a crash during the trial. Must not stop the folder becoming private.
    mkdir "$tmp/save dir/permission-test"; echo x > "$tmp/save dir/permission-test/file"
    cp build-helper/autodeafen-helper helper/autodeafen-start "$tmp/package"
    win() { printf 'Z:%s' "$1" | tr / '\\'; }
    echo "## $wine"
    export WINEPREFIX="$tmp/prefix" WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml,winemenubuilder.exe=" DISPLAY= WAYLAND_DISPLAY=
    timeout 90 "$wine" build-tests/smoke.exe "$id" "$(win "$tmp/package")" "$(win "$tmp/save dir")" 2>&1 |
        grep -E '^(runtime|private|through|connect|reply|idle)' | tr '\n' ';'
    echo; stat -c "%a %n" "$tmp/save dir" "$tmp/save dir"/* 2>/dev/null | sed "s|$tmp/||" | tr '\n' ';'; echo
    "$(dirname "$wine")/wineserver" -k 2>/dev/null; sleep 1
    rm -rf "$tmp"
done
