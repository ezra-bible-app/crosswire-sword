var	argscheck = require('cordova/argscheck'),
	channel   = require('cordova/channel'),
	utils     = require('cordova/utils'),
	exec      = require('cordova/exec'),
	cordova   = require('cordova');

channel.createSticky('onSWORDReady');
// Tell cordova channel to wait on the CordovaInfoReady event
channel.waitForInitialization('onSWORDReady');


function InstallMgr() {
}

InstallMgr.prototype.setUserDisclaimerConfirmed = function(callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "InstallMgr_setUserDisclaimerConfirmed", []
	);
	return retVal;
}

InstallMgr.prototype.syncConfig = function(callback, progressNotify) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "InstallMgr_syncConfig", [progressNotify]
	);
	return retVal;
}


InstallMgr.prototype.getRemoteSources = function(callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "InstallMgr_getRemoteSources", []
	);
	return retVal;
}


InstallMgr.prototype.refreshRemoteSource = function(sourceName, callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "InstallMgr_refreshRemoteSource", [sourceName]
	);
	return retVal;
}

InstallMgr.prototype.getRemoteModInfoList = function(sourceName, callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "InstallMgr_getRemoteModInfoList", [sourceName]
	);
	return retVal;
}

InstallMgr.prototype.getRemoteModuleByName = function(sourceName, modName, callback) {
	var mod = null;
	exec(function(m) { if (m && m.name) mod = new SWModule(m); if (callback) callback(mod); },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "InstallMgr_getRemoteModuleByName", [sourceName, modName]
	);
	return mod;
}

// callback({ status : preStatus|update|complete, totalBytes : n, completedBytes : n, message : displayable });
InstallMgr.prototype.remoteInstallModule = function(sourceName, modName, callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "InstallMgr_remoteInstallModule", [sourceName, modName]
	);
	return retVal;
}

InstallMgr.prototype.uninstallModule = function(modName, callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "InstallMgr_uninstallModule", [modName]
	);
	return retVal;
}

function SWModule(modInfo) {
	this.name             = modInfo.name;
	this.description      = modInfo.description;
	this.category         = modInfo.category;
	this.direction        = modInfo.direction;
	this.language         = modInfo.language;
	this.font             = modInfo.font;
	this.shortCopyright   = modInfo.shortCopyright;
	this.shortPromo       = modInfo.shortPromo;
	this.cipherKey        = modInfo.cipherKey;
	this.remoteSourceName = modInfo.remoteSourceName;
}

SWModule.prototype.SEARCHTYPE_REGEX     =  1;
SWModule.prototype.SEARCHTYPE_PHRASE    = -1;
SWModule.prototype.SEARCHTYPE_MULTIWORD = -2;
SWModule.prototype.SEARCHTYPE_ENTRYATTR = -3;
SWModule.prototype.SEARCHTYPE_LUCENE    = -4;

SWModule.prototype.SEARCHOPTION_ICASE   = 2;
// search flags (SWModule::SEARCHFLAG_*)
SWModule.prototype.SEARCHFLAG_MATCHWHOLEENTRY  = 4096;	// entry-attribute searches: the whole value, not a substring
SWModule.prototype.SEARCHFLAG_STRICTBOUNDARIES = 8192;	// multiword/regex: no hits across verse boundaries


// callback(error): the key's error for this positioning, 0 for none, e.g.
// KEYERR_OUTOFBOUNDS (1) when keyText names nothing in the module's
// versification -- the key has then gone to its bound (asking a Hebrew Bible
// for James leaves it at the first verse of its last book).  The answer comes
// with the call itself; popError afterwards still returns it, but that is a
// second call and others may be scheduled on the module between the two.
SWModule.prototype.setKeyText = function(keyText, callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_setKeyText", [this.name, keyText]
	);
	return retVal;
}

SWModule.prototype.terminateSearch = function(callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_terminateSearch", [this.name]
	);
	return retVal;
}

SWModule.prototype.search = function(expression, searchType, flags, scope, callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_search", [this.name, expression, searchType, flags, scope]
	);
	return retVal;
}

SWModule.prototype.getKeyText = function(callback) {
	var retVal = null;
	exec(callback?callback:function(m) { retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getKeyText", [this.name]
	);
	return retVal;
}

SWModule.prototype.getRenderText = function(callback) {
	var retVal = null;
	exec(callback?callback:function(m) { retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getRenderText", [this.name]
	);
	return retVal;
}

SWModule.prototype.getStripText = function(callback) {
	var retVal = null;
	exec(callback?callback:function(m) { retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getStripText", [this.name]
	);
	return retVal;
}

SWModule.prototype.getRawEntry = function(callback) {
	var retVal = null;
	exec(callback?callback:function(m) { retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getRawEntry", [this.name]
	);
	return retVal;
}

SWModule.prototype.shareVerse = function(callback) {
	var retVal = null;
	exec(callback?callback:function(m) { retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_sendText", [this.name]
	);
	return retVal;
}

/*
 * masterMod - SWModule to use for traversing chapter; null if self
 * 	useful for parallel display with alternate v11ns
 *
 * returns [ { verse : verseKey,  preVerse : 'text', text : 'text' }, ... }
 */
SWModule.prototype.getRenderChapter = function(masterMod, callback) {
	var retVal = null;
	if (!masterMod) masterMod = this;
	exec(callback?callback:function(m) { retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getRenderChapter", [masterMod.name, this.name]
	);
	return retVal;
}

SWModule.prototype.getRenderHeader = function(callback) {
	var retVal = null;
	exec(callback?callback:function(m) { retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getRenderHeader", [this.name]
	);
	return retVal;
}

SWModule.prototype.getKeyChildren = function(callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getKeyChildren", [this.name]
	);
	return retVal;
}

SWModule.prototype.getVerseKey = function(callback) {
	var retVal = {};
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getVerseKey", [this.name]
	);
	return retVal;
}

SWModule.prototype.getConfigEntry = function(key, callback) {
	var retVal = '';
	exec(callback?callback:function(m) { if (m) retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getConfigEntry", [this.name, key, this.remoteSourceName]
	);
	return retVal;
}


SWModule.prototype.popError = function(callback) {
	var retVal = 0;
	exec(callback?callback:function(m) { retVal = m; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_popError", [this.name]
	);
	return retVal;
}

SWModule.prototype.getEntryAttribute = function(level1Key, level2Key, level3Key, isFiltered, callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getEntryAttribute", [this.name, level1Key, level2Key, level3Key, isFiltered]
	);
	return retVal;
}

SWModule.prototype.next = function(callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_next", [this.name]
	);
	return retVal;
}

SWModule.prototype.previous = function(callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_previous", [this.name]
	);
	return retVal;
}

SWModule.prototype.begin = function(callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_begin", [this.name]
	);
	return retVal;
}

SWModule.prototype.getBookNames = function(callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_getBookNames", [this.name]
	);
	return retVal;
}

SWModule.prototype.parseKeyList = function(keyText, callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_parseKeyList", [this.name, keyText]
	);
	return retVal;
}

SWModule.prototype.setRawEntry = function(entryText, callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWModule_setRawEntry", [this.name, entryText]
	);
	return retVal;
}


function SWMgr() {
}


SWMgr.prototype.getModInfoList = function(callback) {
	if (!this.hasOwnProperty('_lastModInfoList')) this._lastModInfoList = [];
	exec(function(m) { if (m && m.length > 0) this._lastModInfoList = m; if (callback) callback(m); },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getModInfoList", []
	);
	return this._lastModInfoList;
}

SWMgr.prototype.getModuleByName = function(modName, callback) {
	var mod = null;
	exec(function(m) { if (m && m.name) mod = new SWModule(m); if (callback) callback(mod); },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getModuleByName", [modName]
	);
	return mod;
}

SWMgr.prototype.getExtraConfigSections = function(callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getExtraConfigSections", []
	);
	return retVal;
}

SWMgr.prototype.getExtraConfigKeys = function(section, callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getExtraConfigKeys", [section]
	);
	return retVal;
}

SWMgr.prototype.getExtraConfigValue = function(section, key, callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getExtraConfigValue", [section, key]
	);
	return retVal;
}

SWMgr.prototype.setExtraConfigValue = function(section, key, value, callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_setExtraConfigValue", [section, key, value]
	);
}

SWMgr.prototype.addExtraConfig = function(confBlob, callback) {
	var retVal = null;
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_addExtraConfig", [confBlob]
	);
	return retVal;
}

SWMgr.prototype.startBibleSync = function(appName, userName, passphrase, callback) {
	exec(callback,
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_startBibleSync", [appName, userName, passphrase]
	);
}

SWMgr.prototype.stopBibleSync = function() {
	exec(function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_stopBibleSync", []
	);
}

SWMgr.prototype.sendBibleSyncMessage = function(osisRef, callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_sendBibleSyncMessage", [osisRef]
	);
}

SWMgr.prototype.setJavascript = function(val, callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_setJavascript", [val]
	);
}

SWMgr.prototype.getAvailableLocales = function(callback) {
	var retVal = [];
	exec(callback?callback:function(r) { retVal = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getAvailableLocales", []
	);
	return retVal;
}

SWMgr.prototype.setDefaultLocale = function(val, callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_setDefaultLocale", [val]
	);
}

SWMgr.prototype.translate = function(text, locale, callback) {
	// support overloaded (text, callback)
	if (!callback && locale) { callback = locale; locale = null; }
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_translate", [text, locale]
	);
}

// The engine's global text options ("Strong's Numbers", "Headings",
// "Words of Christ in Red", ...): the names on offer given the installed
// modules, the values each accepts ("On"/"Off" or a list), the current
// value, and a one-line description.
SWMgr.prototype.getGlobalOptions = function(callback) {
	exec(callback?callback:function() {},
		function(err) { if (callback) callback([]); },
		"SWORD", "SWMgr_getGlobalOptions", []
	);
}

SWMgr.prototype.getGlobalOptionValues = function(option, callback) {
	exec(callback?callback:function() {},
		function(err) { if (callback) callback([]); },
		"SWORD", "SWMgr_getGlobalOptionValues", [option]
	);
}

SWMgr.prototype.getGlobalOption = function(option, callback) {
	exec(callback?callback:function() {},
		function(err) { if (callback) callback(''); },
		"SWORD", "SWMgr_getGlobalOption", [option]
	);
}

SWMgr.prototype.getGlobalOptionTip = function(option, callback) {
	exec(callback?callback:function() {},
		function(err) { if (callback) callback(''); },
		"SWORD", "SWMgr_getGlobalOptionTip", [option]
	);
}

SWMgr.prototype.setGlobalOption = function(option, value, callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_setGlobalOption", [option, value]
	);
}

// the sword:// or bible:// link the app was opened with, once; '' when none
SWMgr.prototype.getLaunchUrl = function(callback) {
	exec(callback, function(err) { utils.alert('[ERROR] problem: ' + err); }, "SWORD", "SWMgr_getLaunchUrl", []);
}

SWMgr.prototype.getPrefixPath = function(callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getPrefixPath", []
	);
}

// A folder shared between SWORD apps used as the module library (Android:
// a Storage Access Framework document tree).  Calls back with its uri, or ""
// if the app is using its private storage only.
SWMgr.prototype.getSharedStorage = function(callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getSharedStorage", []
	);
}

// Let the user pick (or create) a shared library folder.  SWORD is
// re-initialized on it.  Calls back with the folder's uri, or "" if the user
// cancelled.  Module lists should be refreshed afterward.
SWMgr.prototype.chooseSharedStorage = function(callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_chooseSharedStorage", []
	);
}

// Stop using a shared library folder; back to private storage only.
SWMgr.prototype.clearSharedStorage = function(callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_clearSharedStorage", []
	);
}

// Where the app should keep small per-user study data (bookmarks/, ...):
// the separately chosen data folder if there is one, else beside the module
// library.  Ends in '/'.
SWMgr.prototype.getDataPath = function(callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getDataPath", []
	);
}

// The study-data folder chosen separately from the library (Android: a SAF
// document tree, which may live on a cloud provider such as Drive).  Calls
// back with its uri, or "" if study data lives beside the library.
SWMgr.prototype.getDataStorage = function(callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_getDataStorage", []
	);
}

// Let the user pick a study-data folder.  SWORD is re-initialized.  Calls
// back with the folder's uri, or "" if the user cancelled.
SWMgr.prototype.chooseDataStorage = function(callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_chooseDataStorage", []
	);
}

// A human label for a granted folder uri, "<provider app> / <folder>"
// (e.g. "Drive / sword-bookmarks"); "" if it can't be described.
SWMgr.prototype.describeTree = function(uri, callback) {
	exec(callback?callback:function() {},
		function(err) { if (callback) callback(''); },
		"SWORD", "SWMgr_describeTree", [uri]
	);
}

// Keep study data beside the module library again.
SWMgr.prototype.clearDataStorage = function(callback) {
	exec(callback?callback:function() {},
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "SWMgr_clearDataStorage", []
	);
}

// Plain file access, routed through the engine's FileMgr so that paths under
// the shared library (getPrefixPath()) work whether it is private storage or
// a SAF folder.  Meant for small user data kept beside the library, e.g.
// bookmarks/*.conf.  Each calls back (result, err); err is set on failure
// and is never reported with an alert.
SWMgr.prototype.readFile = function(path, callback) {
	exec(function(contents) { if (callback) callback(contents); },
		function(err) { if (callback) callback(null, err); },
		"SWORD", "SWMgr_readFile", [path]
	);
}

SWMgr.prototype.writeFile = function(path, contents, callback) {
	exec(function() { if (callback) callback(true); },
		function(err) { if (callback) callback(false, err); },
		"SWORD", "SWMgr_writeFile", [path, contents]
	);
}

// Entry names; subdirectories carry a trailing "/".
SWMgr.prototype.listDir = function(path, callback) {
	exec(function(names) { if (callback) callback(names); },
		function(err) { if (callback) callback([], err); },
		"SWORD", "SWMgr_listDir", [path]
	);
}

SWMgr.prototype.removeFile = function(path, callback) {
	exec(function() { if (callback) callback(true); },
		function(err) { if (callback) callback(false, err); },
		"SWORD", "SWMgr_removeFile", [path]
	);
}

SWMgr.prototype.mkdir = function(path, callback) {
	exec(function() { if (callback) callback(true); },
		function(err) { if (callback) callback(false, err); },
		"SWORD", "SWMgr_mkdir", [path]
	);
}

function HTTPUtils() {}

HTTPUtils.prototype.METHOD_GET  =  0;
HTTPUtils.prototype.METHOD_POST =  1;

HTTPUtils.prototype.makeRequest = function(url, postData, callback, method) {
	var result = null;
	if (!method) method = this.METHOD_POST;
	exec(callback?callback:function(r) { if (r && r.length > 0) result = r; },
		function(err) { utils.alert('[ERROR] problem: ' + err); },
		"SWORD", "HTTPUtils_makeRequest", [url, postData, method]
	);
	return result;
}



/*
	public native String      getConfigPath();
	public native String      getGlobalOption(String option);
	public native String      getGlobalOptionTip(String option);
	public native String      filterText(String filterName, String text);
	public native String[]    getGlobalOptions();
	public native String[]    getGlobalOptionValues(String option);
	public native void        setCipherKey(String modName, String key);
	public native void        setJavascript(boolean val);
*/


/**
 * This is the SWORD namespace and access to singleton SWMgr, InstallMgr, and HTTPUtils.
 * @constructor
 */
function SWORD() {

	var me = this;

	this.available = false;
	this.version = null;

	this.installMgr = new InstallMgr();
	this.mgr        = new SWMgr();
	this.httpUtils  = new HTTPUtils();

	channel.onCordovaReady.subscribe(function() {
		me.init(function(info) {
			me.available = true;
			me.version = info.version;
			channel.onSWORDReady.fire();
		},function(e) {
			me.available = false;
			utils.alert("[ERROR] Error initializing SWORD: " + e);
		});
	});
}


SWORD.prototype.CATEGORY_BIBLES       = "Biblical Texts";
SWORD.prototype.CATEGORY_COMMENTARIES = "Commentaries";
SWORD.prototype.CATEGORY_LEXDICTS     = "Lexicons / Dictionaries";
SWORD.prototype.CATEGORY_GENBOOKS     = "Generic Books";
SWORD.prototype.CATEGORY_DAILYDEVOS   = "Daily Devotional";
SWORD.prototype.osisOT                = ['Gen', 'Exod', 'Lev', 'Num', 'Deut', 'Josh', 'Judg', 'Ruth', '1Sam', '2Sam', '1Kgs', '2Kgs', '1Chr', '2Chr', 'Ezra', 'Neh', 'Esth', 'Job', 'Ps', 'Prov', 'Eccl', 'Song', 'Isa', 'Jer', 'Lam', 'Ezek', 'Dan', 'Hos', 'Joel', 'Amos', 'Obad', 'Jonah', 'Mic', 'Nah', 'Hab', 'Zeph', 'Hag', 'Zech', 'Mal'];
SWORD.prototype.osisNT                = ['Matt', 'Mark', 'Luke', 'John', 'Acts', 'Rom', '1Cor', '2Cor', 'Gal', 'Eph', 'Phil', 'Col', '1Thess', '2Thess', '1Tim', '2Tim', 'Titus', 'Phlm', 'Heb', 'Jas', '1Pet', '2Pet', '1John', '2John', '3John', 'Jude', 'Rev'];


SWORD.prototype.init = function(successCallback, errorCallback) {
	exec(successCallback, errorCallback, "SWORD", "initSWORD", []);
};


module.exports = new SWORD();


