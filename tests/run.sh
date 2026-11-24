#!/bin/sh
# Native tests for the parts that do not need GD or Discord.
set -e
cd "$(dirname "$0")/.."
mkdir -p build-tests
clang++ -std=c++23 -Wall -Wextra -Werror -g -fsanitize=address,undefined -o build-tests/run tests/run.cpp
build-tests/run
