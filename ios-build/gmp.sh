#!/bin/sh
# Cross-compile GMP 6.3.0 for iOS ARM64.
set -e
WORK=${1:-/tmp/gmp-ios-build}
PREFIX=${2:-/tmp/gmp-ios-install}
GMP_VERSION=6.3.0
GMP_SHA256=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898

mkdir -p "$WORK" && cd "$WORK"

if [ ! -f gmp-${GMP_VERSION}.tar.xz ]; then
    curl -sL "https://gmplib.org/download/gmp/gmp-${GMP_VERSION}.tar.xz" -o gmp-${GMP_VERSION}.tar.xz
fi

# Fail closed on checksum mismatch. Supply-chain guard: a hijacked mirror,
# poisoned CI cache, or on-path MITM cannot silently swap the tarball.
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
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
export CC="$(xcrun --sdk iphoneos --find clang)"
export CFLAGS="-arch arm64 -isysroot $SDK -miphoneos-version-min=17.0"
export LDFLAGS="-arch arm64 -isysroot $SDK -miphoneos-version-min=17.0"
./configure --host=aarch64-apple-darwin --prefix="$PREFIX" \
    --disable-shared --enable-static --disable-assembly
make -j8
make install
echo "GMP installed at $PREFIX"
