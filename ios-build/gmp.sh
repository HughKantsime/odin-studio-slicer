#!/bin/sh
# Cross-compile GMP 6.3.0 for iOS ARM64.
set -e
WORK=${1:-/tmp/gmp-ios-build}
PREFIX=${2:-/tmp/gmp-ios-install}
mkdir -p "$WORK" && cd "$WORK"
[ -f gmp-6.3.0.tar.xz ] || curl -sL https://gmplib.org/download/gmp/gmp-6.3.0.tar.xz -o gmp-6.3.0.tar.xz
[ -d gmp-6.3.0 ] || tar xf gmp-6.3.0.tar.xz
cd gmp-6.3.0
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
export CC="$(xcrun --sdk iphoneos --find clang)"
export CFLAGS="-arch arm64 -isysroot $SDK -miphoneos-version-min=17.0"
export LDFLAGS="-arch arm64 -isysroot $SDK -miphoneos-version-min=17.0"
./configure --host=aarch64-apple-darwin --prefix="$PREFIX" \
    --disable-shared --enable-static --disable-assembly
make -j8
make install
echo "GMP installed at $PREFIX"
