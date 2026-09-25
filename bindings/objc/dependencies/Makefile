LIBCURLVERSION=7.57.0
ICUVERSION=53.1
CLUCENEVERSION=9.21b

# the prebuilt iOS SWORD.xcframework for the cordova plugin (its server
# folder has a README saying how it was built); iOS needs no curl: HTTP(S)
# goes through NSURLSession (NSURLHTTPTransport), FTP through ftplib
IOSSWORD=sword-ios-latest
PLUGINLIBS=../../cordova/cordova-plugin-crosswire-sword/libs/ios

APPLEDEPS=https://crosswire.org/ftpmirror/pub/sword/dependencies/apple

all: curl/unpacked icu/unpacked clucene/unpacked ios

clean: curlclean icuclean cluceneclean iosclean

libcurl_src.tar.gz:
	curl -fsSLO ${APPLEDEPS}/libcurl-${LIBCURLVERSION}/libcurl_src.tar.gz

clucene_src.tar.gz:
	curl -fsSLO ${APPLEDEPS}/clucene-${CLUCENEVERSION}/clucene_src.tar.gz

icu_src.tar.gz:
	curl -fsSLO ${APPLEDEPS}/icu-${ICUVERSION}/icu_src.tar.gz


curl/unpacked: libcurl_src.tar.gz
	tar xjfv libcurl_src.tar.gz
	touch curl/unpacked

icu/unpacked: icu_src.tar.gz
	tar xzfv icu_src.tar.gz
	touch icu/unpacked

clucene/unpacked: clucene_src.tar.gz
	tar xzfv clucene_src.tar.gz
	touch clucene/unpacked

ios: ${PLUGINLIBS}/SWORD.xcframework

${PLUGINLIBS}/SWORD.xcframework:
	mkdir -p ${PLUGINLIBS}
	curl -fsSL ${APPLEDEPS}/${IOSSWORD}/SWORD.xcframework.tar.gz | tar xzf - -C ${PLUGINLIBS}

curlclean:
	rm -rf libcurl* curl

icuclean:
	rm -rf icu*

cluceneclean:
	rm -rf clucene*

iosclean:
	rm -rf ${PLUGINLIBS}/curl.xcframework ${PLUGINLIBS}/SWORD.xcframework

