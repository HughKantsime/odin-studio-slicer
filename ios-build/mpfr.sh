#!/bin/sh
# Cross-compile MPFR 4.2.1 for iOS ARM64. Requires GMP already installed.
set -e
WORK=${1:-/tmp/mpfr-ios-build}
GMP=${2:-/tmp/gmp-ios-install}
PREFIX=${3:-/tmp/mpfr-ios-install}
mkdir -p "$WORK" && cd "$WORK"
[ -f mpfr-4.2.1.tar.xz ] || curl -sL https://www.mpfr.org/mpfr-4.2.1/mpfr-4.2.1.tar.xz -o mpfr-4.2.1.tar.xz
[ -d mpfr-4.2.1 ] || tar xf mpfr-4.2.1.tar.xz
cd mpfr-4.2.1
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
export CC="$(xcrun --sdk iphoneos --find clang)"
export CFLAGS="-arch arm64 -isysroot $SDK -miphoneos-version-min=17.0"
export LDFLAGS="-arch arm64 -isysroot $SDK -miphoneos-version-min=17.0"
./configure --host=aarch64-apple-darwin --prefix="$PREFIX" --with-gmp="$GMP" \
    --disable-shared --enable-static
make -j8
make install
echo "MPFR installed at $PREFIX"
