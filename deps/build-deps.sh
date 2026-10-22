#!/bin/sh
# Fetch and build the pinned third-party libraries, statically, into deps/prefix.
# Each is pinned by tag AND by the commit that tag must resolve to.
#
#   OpenSSL    3.5.9  (LTS line)  SHA-256, SHA3/SHAKE, SLH-DSA (FIPS 205), ML-KEM-768 (FIPS 203), X25519
#   libsodium  1.0.21             XChaCha20-Poly1305 (IETF construction)
#   secp256k1  0.7.1              BIP-340 Schnorr verification (and signing, for test reproduction)
#   utf8proc   2.11.3             Unicode 17.0.0 normalization tables (Text rule 12/13)
set -eu
here=$(cd "$(dirname "$0")" && pwd)
src="$here/src"
prefix="$here/prefix"
mkdir -p "$src" "$prefix"
jobs=$(nproc 2>/dev/null || echo 4)

fetch() { # name url tag commit
	if [ ! -d "$src/$1" ]; then
		git -c advice.detachedHead=false clone -q --depth 1 --branch "$3" "$2" "$src/$1"
	fi
	got=$(git -C "$src/$1" rev-parse HEAD)
	if [ "$got" != "$4" ]; then
		echo "pin mismatch for $1: $got != $4" >&2
		exit 1
	fi
}

fetch openssl   https://github.com/openssl/openssl.git          openssl-3.5.9    45e844fa2a14ec92d146bd8f5778ac130b6625fb
fetch libsodium https://github.com/jedisct1/libsodium.git       1.0.21-RELEASE   d24faf56214469b354b01c8ba36257e04737101e
fetch secp256k1 https://github.com/bitcoin-core/secp256k1.git   v0.7.1           1a53f4961f337b4d166c25fce72ef0dc88806618
fetch utf8proc  https://github.com/JuliaStrings/utf8proc.git    v2.11.3          e5e799221b45bbb90f5fdc5c69b6b8dfbf017e78

if [ ! -f "$prefix/lib/libcrypto.a" ]; then
	(cd "$src/openssl" && ./Configure no-shared no-tests no-docs no-apps no-module \
		--prefix="$prefix" --libdir=lib -fPIC >/dev/null && make -s -j"$jobs" build_libs >/dev/null \
		&& make -s install_dev >/dev/null)
fi
if [ ! -f "$prefix/lib/libsodium.a" ]; then
	(cd "$src/libsodium" && ./configure -q --disable-shared --enable-static --with-pic --prefix="$prefix" >/dev/null \
		&& make -s -j"$jobs" >/dev/null && make -s install >/dev/null)
fi
if [ ! -f "$prefix/lib/libsecp256k1.a" ]; then
	cmake -S "$src/secp256k1" -B "$src/secp256k1/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=OFF -DSECP256K1_ENABLE_MODULE_SCHNORRSIG=ON -DSECP256K1_ENABLE_MODULE_EXTRAKEYS=ON \
		-DSECP256K1_BUILD_TESTS=OFF -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF -DSECP256K1_BUILD_BENCHMARK=OFF \
		-DSECP256K1_BUILD_EXAMPLES=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
		-DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_INSTALL_LIBDIR=lib >/dev/null
	cmake --build "$src/secp256k1/build" >/dev/null
	cmake --install "$src/secp256k1/build" >/dev/null
fi
if [ ! -f "$prefix/lib/libutf8proc.a" ]; then
	cmake -S "$src/utf8proc" -B "$src/utf8proc/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
		-DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_INSTALL_LIBDIR=lib >/dev/null
	cmake --build "$src/utf8proc/build" >/dev/null
	cmake --install "$src/utf8proc/build" >/dev/null
fi
echo "deps ready in $prefix"
