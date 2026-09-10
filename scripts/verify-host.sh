#!/usr/bin/env sh
set -eu

for preset in host-debug host-release host-sanitize; do
    cmake --preset "$preset" -DBRSP_WARNINGS_AS_ERRORS=ON
    cmake --build --preset "$preset"
    ctest --preset "$preset"
done
./build/host-release/examples/brsp_example_solve
./build/host-release/examples/brsp_example_sparse
cmake --install build/host-release --prefix "$PWD/build/install"
cmake -S tests/consumer -B build/consumer -G Ninja -DCMAKE_PREFIX_PATH="$PWD/build/install"
cmake --build build/consumer
./build/consumer/brsp_consumer
./build/consumer/brsp_consumer_ordered
