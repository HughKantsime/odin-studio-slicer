#!/bin/sh
# Cross-compile GMP 6.3.0 for iOS simulator (arm64-apple-ios-simulator).
# Mirrors gmp.sh; different sysroot + target flag. Checksum-pinned.
set -e
WORK=${1:-/tmp/gmp-ios-sim-build}
PREFIX=${2:-/tmp/gmp-ios-sim-install}
GMP_VERSION=6.3.0
GMP_SHA256=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898

mkdir -p "$WORK" && cd "$WORK"

if [ ! -f gmp-${GMP_VERSION}.tar.xz ]; then
    curl -sL "https://gmplib.org/download/gmp/gmp-${GMP_VERSION}.tar.xz" -o gmp-${GMP_VERSION}.tar.xz
fi

ACTUAL=$(shasum -a 256 gmp-${GMP_VERSION}.tar.xz | awk '{print $1}')
if [ "$ACTUAL" != "$GMP_SHA256" ]; then
    echo "GMP tarball checksum mismatch." >&2
    echo "  expected: $GMP_SHA256" >&2
    echo "  got:      $ACTUAL" >&2
    rm -f gmp-${GMP_VERSION}.tar.xz
    exit 1
fi

[ -d gmp-${GMP_VERSION} ] || tar xf gmp-${GMP_VERSION}.tar.xz
cd gmp-${GMP_VERSION}
SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)
export CC="$(xcrun --sdk iphonesimulator --find clang)"
# -target switches Clang between device/simulator platform bits; autoconf
# still needs --host so it believes we're cross-compiling.
export CFLAGS="-arch arm64 -isysroot $SDK -target arm64-apple-ios17.0-simulator"
export LDFLAGS="-arch arm64 -isysroot $SDK -target arm64-apple-ios17.0-simulator"
./configure --host=aarch64-apple-darwin --prefix="$PREFIX" \
    --disable-shared --enable-static --disable-assembly
make -j8
make install
echo "GMP (simulator) installed at $PREFIX"
