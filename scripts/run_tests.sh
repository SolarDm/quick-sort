#!/usr/bin/env bash
set -euo pipefail

ROOT="$(realpath "$(dirname "$0")/..")"
BUILD="$ROOT/build-sanitize"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build "$BUILD" --target correctness_tests --parallel 4
ctest --test-dir "$BUILD" --output-on-failure
