LIBCURLVERSION=7.57.0
ICUVERSION=53.1
CLUCENEVERSION=9.21b

# prebuilt iOS xcframeworks for the cordova plugin (each server folder has a
# README saying how it was built; build-curl-ios.sh here rebuilds curl's)
IOSCURL=curl-8.22.0-openssl-3.5.8
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

ios: ${PLUGINLIBS}/curl.xcframework ${PLUGINLIBS}/SWORD.xcframework

${PLUGINLIBS}/curl.xcframework:
	mkdir -p ${PLUGINLIBS}
	curl -fsSL ${APPLEDEPS}/${IOSCURL}/curl.xcframework.tar.gz | tar xzf - -C ${PLUGINLIBS}

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

