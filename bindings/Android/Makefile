# The plugin's Android native libraries (libsword.so and its curl, OpenSSL and
# libc++ companions, one folder per ABI) are build products and are not kept
# in git.  `make` copies them from an Android Studio Release build of SWORD/
# (see README); `make fetch` downloads the prebuilt set published on
# crosswire.org instead (the server folder's README says how it was built).
SWORDANDROID=sword-android-ea56bca4
ANDROIDDEPS=https://crosswire.org/ftpmirror/pub/sword/dependencies/android
PLUGINLIBS=../cordova/cordova-plugin-crosswire-sword/libs

all: javawrapper
	mkdir -p ${PLUGINLIBS}/android
	rm -rf ${PLUGINLIBS}/android/*
	cp -a SWORD/app/build/intermediates/stripped_native_libs/release/stripReleaseDebugSymbols/out/lib/* ${PLUGINLIBS}/android/
#	cp -a SWORD/app/build/intermediates/transforms/stripDebugSymbol/release/0/lib/* ${PLUGINLIBS}/android/

debug: javawrapper
	rm -rf ${PLUGINLIBS}/android/*
	cp -a SWORD/app/build/intermediates/stripped_native_libs/debug/stripDebugDebugSymbols/out/lib/* ${PLUGINLIBS}/android/
#	cp -a SWORD/app/build/intermediates/merged_native_libs/debug/out/lib/* ${PLUGINLIBS}/android/

fetch: ${PLUGINLIBS}/android/arm64-v8a/libsword.so

${PLUGINLIBS}/android/arm64-v8a/libsword.so:
	mkdir -p ${PLUGINLIBS}
	curl -fsSL ${ANDROIDDEPS}/${SWORDANDROID}/libsword-android.tar.gz | tar xzf - -C ${PLUGINLIBS}

fetchclean:
	rm -rf ${PLUGINLIBS}/android

javawrapper:
	cp -a SWORD/app/src/main/java/org/crosswire/android/sword/* ../cordova/cordova-plugin-crosswire-sword/src/android/

clean:
	#these are from ../java-jni
	rm SWORD/app/src/main/java/org/crosswire/android/sword/InstallMgr.java SWORD/app/src/main/java/org/crosswire/android/sword/SWMgr.java SWORD/app/src/main/java/org/crosswire/android/sword/SWModule.java
