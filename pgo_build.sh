#!/bin/bash
set -e

echo "=== Clean Build ==="
rm -rf build default.profraw default.profdata
mkdir build

echo "=== Step 1: Instrument Build (-fprofile-generate) ==="
cd build
cmake .. -DRADIKANT_JSON_PGO_GENERATE=ON -DRADIKANT_JSON_PGO_USE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build .
cd ..

echo "=== Step 2: Generate Profile Data ==="
# Export the profile file path so LLVM knows exactly where to dump it
export LLVM_PROFILE_FILE="$(pwd)/default.profraw"
cd build/test
./TST-JSON-VECTORS
./TST-JSON-PERF
cd ../..

echo "=== Step 3: Merge Profile Data ==="
xcrun llvm-profdata merge -output=default.profdata default.profraw
echo "Profile generated: default.profdata"

echo "=== Step 4: Recompile with Profile (-fprofile-use) ==="
rm -rf build
mkdir build
cd build
cmake .. -DRADIKANT_JSON_PGO_GENERATE=OFF -DRADIKANT_JSON_PGO_USE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build .

echo "=== Step 5: Final Benchmark Run ==="
cd test
./TST-JSON-PERF
