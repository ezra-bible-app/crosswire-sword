#
# First enter dependencies/ and run make: besides the source packages it
# fetches the prebuilt SWORD.xcframework and curl.xcframework into
# ../cordova/cordova-plugin-crosswire-sword/libs/ios/
#
# To build them yourself instead: dependencies/build-curl-ios.sh makes
# curl.xcframework; SWORD.xcframework is target SWORD of SWORD/SWORD.xcodeproj
# built for iphoneos and iphonesimulator, then xcodebuild -create-xcframework
# (the README beside each download on crosswire.org gives the invocations)
#
# This Makefile copies the headers the cordova plugin needs.
#
# just type: make
#

all: deploycordova

deploycordova:
	mkdir -p ../cordova/cordova-plugin-crosswire-sword/src/ios/
	cp ../../include/defs.h ../../include/flatapi.h ../cordova/cordova-plugin-crosswire-sword/src/ios/



