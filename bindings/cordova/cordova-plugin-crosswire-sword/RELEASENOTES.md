<!--
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
-->
# Release Notes

### 0.2.10 (Sep 26, 2026)
* iOS: HTTP(S) through Apple's NSURLSession; no curl or OpenSSL in the app, so App Store Connect's export-compliance questions do not arise (only Apple's encryption)
* Android: a cloud folder (Google Drive) whose provider is still loading is waited for instead of being taken as empty, a listing that fails is reported to the page, and a file another device replaced is found again by name; the app no longer creates a second personal.conf beside the real one
* FTP without curl (iOS): EPSV first and PASV's advertised address ignored, as curl does, so sources such as STEP answer; failures are reported as with curl, so file-by-file installs proceed on sources without packages

### 0.2.9 (Sep 25, 2026)
* BibleSync works: biblesync 2.2.0 with the Android and Apple interface discovery sent upstream; Android holds a Wi-Fi multicast lock while it runs (CHANGE_WIFI_MULTICAST_STATE); every speaker in the room is heard, not only the first
* Android: a sword:// or bible:// link the app is opened with reaches the page as handleOpenURL, exactly as cordova-ios does, on a cold start and a warm one
* iOS: a received BibleSync verse no longer crashes the app
* The native binaries come from the sword-android-latest and sword-ios-latest links on crosswire.org

### 0.2.8 (Aug 22, 2026)
* iOS: builds with cordova-ios 8 and Xcode 27 (xcframeworks for libsword and curl 8.22 with OpenSSL 3.5.8, verified against the iOS trust store)
* The native binaries of both platforms are no longer in git: `make fetch` in bindings/Android and `make ios` in bindings/objc/dependencies download the published sets from crosswire.org
* SWMgr.readFile/writeFile/listDir/removeFile/mkdir: plain file access through the engine's FileMgr, so paths in a shared (SAF) library work
* SWMgr.getDataPath/getDataStorage/chooseDataStorage/clearDataStorage/describeTree: a separately granted study-data folder (e.g. on Google Drive) for bookmarks and the like
* Android: writes to non-local document providers (Drive...) are staged and committed on close; such providers only accept content through a Java-side close
* Android: actions on the same module run in order, different modules in parallel; chapter rendering and search run on engine-side clones (SWModule::clone), so the UI never waits on them
* Android: re-initialisation keeps retiring managers alive briefly instead of deleting them, so renders and searches in flight survive a storage change
* SWModule.SEARCHFLAG_MATCHWHOLEENTRY / SEARCHFLAG_STRICTBOUNDARIES search flag constants
* SWMgr.getLaunchUrl and Android/iOS handling of sword:// and bible:// links (sword://module/key; empty module = the app's current Bible)
* Android: default lexica re-resolved after augmenting modules (crash on the first Strong's render after choosing a shared library)

### 0.2.7 (Aug 19, 2026)
* SWMgr.getSharedStorage/chooseSharedStorage/clearSharedStorage: Android shared module library in a Storage Access Framework folder (e.g. Documents/sword) usable by several SWORD apps
* makeRequest returns the error body on HTTP error statuses

### 0.2.4 (Dec 7, 2020)
* Updated documentation

### 0.2.1 (Nov 24, 2020)
* SWORD 1.9.0 release

