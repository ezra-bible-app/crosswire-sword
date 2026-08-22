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

### 0.2.8 (Aug 22, 2026)
* SWMgr.readFile/writeFile/listDir/removeFile/mkdir: plain file access through the engine's FileMgr, so paths in a shared (SAF) library work
* SWMgr.getDataPath/getDataStorage/chooseDataStorage/clearDataStorage/describeTree: a separately granted study-data folder (e.g. on Google Drive) for bookmarks and the like
* Android: writes to non-local document providers (Drive...) are staged and committed on close; such providers only accept content through a Java-side close
* Android: all SWORD calls serialised through one lock (chapter rendering runs on the thread pool while re-initialisation runs on the main thread)
* Android: default lexica re-resolved after augmenting modules (crash on the first Strong's render after choosing a shared library)

### 0.2.7 (Aug 19, 2026)
* SWMgr.getSharedStorage/chooseSharedStorage/clearSharedStorage: Android shared module library in a Storage Access Framework folder (e.g. Documents/sword) usable by several SWORD apps
* makeRequest returns the error body on HTTP error statuses

### 0.2.4 (Dec 7, 2020)
* Updated documentation

### 0.2.1 (Nov 24, 2020)
* SWORD 1.9.0 release

