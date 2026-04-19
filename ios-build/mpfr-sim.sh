#!/bin/sh
# Cross-compile MPFR 4.2.1 for iOS simulator. Requires GMP (sim) already installed.
set -e
WORK=${1:-/tmp/mpfr-ios-sim-build}
GMP=${2:-/tmp/gmp-ios-sim-install}
PREFIX=${3:-/tmp/mpfr-ios-sim-install}
MPFR_VERSION=4.2.1
MPFR_SHA256=277807353a6726978996945af13e52829e3abd7a9a5b7fb2793894e18f1fcbb2

mkdir -p "$WORK" && cd "$WORK"

if [ ! -f mpfr-${MPFR_VERSION}.tar.xz ]; then
    curl -sL "https://www.mpfr.org/mpfr-${MPFR_VERSION}/mpfr-${MPFR_VERSION}.tar.xz" -o mpfr-${MPFR_VERSION}.tar.xz
fi

ACTUAL=$(shasum -a 256 mpfr-${MPFR_VERSION}.tar.xz | awk '{print $1}')
if [ "$ACTUAL" != "$MPFR_SHA256" ]; then
    echo "MPFR tarball checksum mismatch." >&2
    echo "  expected: $MPFR_SHA256" >&2
    echo "  got:      $ACTUAL" >&2
    rm -f mpfr-${MPFR_VERSION}.tar.xz
    exit 1
fi

[ -d mpfr-${MPFR_VERSION} ] || tar xf mpfr-${MPFR_VERSION}.tar.xz
cd mpfr-${MPFR_VERSION}
SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)
export CC="$(xcrun --sdk iphonesimulator --find clang)"
export CFLAGS="-arch arm64 -isysroot $SDK -target arm64-apple-ios17.0-simulator"
export LDFLAGS="-arch arm64 -isysroot $SDK -target arm64-apple-ios17.0-simulator"
./configure --host=aarch64-apple-darwin --prefix="$PREFIX" --with-gmp="$GMP" \
    --disable-shared --enable-static
make -j8
make install
echo "MPFR (simulator) installed at $PREFIX"
