#!/bin/sh
# Build libcurl (on OpenSSL, verifying against the iOS trust store) for
# iOS device arm64 and simulator arm64, and package it as curl.xcframework.
# Needs full Xcode selected, and curl-8.22.0/ and openssl-3.5.8/ unpacked
# beside this script (from curl.se and openssl.org).
set -e
cd "$(dirname "$0")"
TOP=$PWD
CURL=curl-8.22.0
SSL=openssl-3.5.8
MIN=15.0
OUT=$TOP/out

build() {	# $1 sdk, $2 target triple, $3 openssl Configure target, $4 min-version flag
	SDK=$1; TRIPLE=$2; SSLTARGET=$3; MINFLAG=$4; P=$OUT/$SDK
	SYSROOT=$(xcrun --sdk $SDK --show-sdk-path)
	CC="$(xcrun --sdk $SDK -f clang)"
	CFLAGS="-arch arm64 -target $TRIPLE -isysroot $SYSROOT -O2"
	rm -rf $P && mkdir -p $P

	rm -rf build-$SSL-$SDK && cp -R $SSL build-$SSL-$SDK
	( cd build-$SSL-$SDK && ./Configure $SSLTARGET --prefix=$P --libdir=lib \
		no-shared no-apps no-docs no-tests no-engine no-module no-dso no-async \
		$MINFLAG=$MIN \
	  && make -j8 build_libs && make install_dev )

	rm -rf build-$CURL-$SDK && cp -R $CURL build-$CURL-$SDK
	( cd build-$CURL-$SDK && ./configure --host=arm64-apple-darwin --prefix=$P \
		CC="$CC" CFLAGS="$CFLAGS" CPPFLAGS="-arch arm64 -target $TRIPLE -isysroot $SYSROOT" LDFLAGS="-arch arm64 -target $TRIPLE -isysroot $SYSROOT" \
		--disable-shared --enable-static --with-openssl=$P --with-apple-sectrust \
		--without-ca-bundle --without-ca-path --without-ca-fallback \
		--without-libpsl --without-brotli --without-zstd --without-nghttp2 --without-libidn2 \
		--disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet \
		--disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-gopher \
		--disable-mqtt --disable-smb --disable-docs --disable-manual --disable-ntlm \
		&& make -j8 -C lib && make -C lib install && make -C include install )

	# one static lib per platform: curl plus the OpenSSL pieces it needs
	libtool -static -o $P/libcurl-all.a $P/lib/libcurl.a $P/lib/libssl.a $P/lib/libcrypto.a
	rm -rf $P/hdr && mkdir -p $P/hdr && cp -R $P/include/curl $P/hdr/
}

build iphoneos arm64-apple-ios$MIN ios64-xcrun -mios-version-min
build iphonesimulator arm64-apple-ios$MIN-simulator iossimulator-arm64-xcrun -mios-simulator-version-min

rm -rf $OUT/curl.xcframework
xcodebuild -create-xcframework \
	-library $OUT/iphoneos/libcurl-all.a -headers $OUT/iphoneos/hdr \
	-library $OUT/iphonesimulator/libcurl-all.a -headers $OUT/iphonesimulator/hdr \
	-output $OUT/curl.xcframework
echo "done: $OUT/curl.xcframework"
