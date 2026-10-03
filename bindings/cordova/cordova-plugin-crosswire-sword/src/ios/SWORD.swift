import WebKit
import UniformTypeIdentifiers

var mySWORDPlugin:SWORD? = nil


@objc(SWORD) class SWORD : CDVPlugin, UIDocumentPickerDelegate {
	var mgr = 0;
	var installMgr = 0
	var disclaimerConfirmed = false;

	var VERSEKEY_BOOK = Int(org_crosswire_sword_SWModule_VERSEKEY_BOOK);
	var VERSEKEY_CHAPTER = Int(org_crosswire_sword_SWModule_VERSEKEY_CHAPTER);
	var VERSEKEY_VERSE = Int(org_crosswire_sword_SWModule_VERSEKEY_VERSE);
	var VERSEKEY_TESTAMENT = Int(org_crosswire_sword_SWModule_VERSEKEY_TESTAMENT);
	var VERSEKEY_OSISREF = Int(org_crosswire_sword_SWModule_VERSEKEY_OSISREF);
	var VERSEKEY_CHAPTERMAX = Int(org_crosswire_sword_SWModule_VERSEKEY_CHAPTERMAX);
	var VERSEKEY_VERSEMAX = Int(org_crosswire_sword_SWModule_VERSEKEY_VERSEMAX);
	var VERSEKEY_BOOKNAME = Int(org_crosswire_sword_SWModule_VERSEKEY_BOOKNAME);
	var VERSEKEY_SHORTTEXT = Int(org_crosswire_sword_SWModule_VERSEKEY_SHORTTEXT);
	var VERSEKEY_BOOKABBREV = Int(org_crosswire_sword_SWModule_VERSEKEY_BOOKABBREV);
	var VERSEKEY_OSISBOOKNAME = Int(org_crosswire_sword_SWModule_VERSEKEY_OSISBOOKNAME);
	var VERSEKEY_COLLECTIONNAME = Int(org_crosswire_sword_SWModule_VERSEKEY_COLLECTIONNAME);

    var LOG_ERROR = Int32(org_crosswire_sword_SWLog_LOG_ERROR);
    var LOG_WARN = Int32(org_crosswire_sword_SWLog_LOG_WARN);
    var LOG_INFO = Int32(org_crosswire_sword_SWLog_LOG_INFO);
    var LOG_TIMEDINFO = Int32(org_crosswire_sword_SWLog_LOG_TIMEDINFO);
    var LOG_DEBUG = Int32(org_crosswire_sword_SWLog_LOG_DEBUG);

	// who is asking, for the servers we talk to, shaped as on Android:
	// "Bishop/1.7.908 (Apple iPhone14,7; iOS 26.5) SWORD/1.9.0.3981".  Goes
	// on every request makeRequest makes, and is appended to the WebView's
	// own User-Agent.
	static var userAgent: String? = nil

	func buildUserAgent(engineVersion: String) -> String {
		let info = Bundle.main.infoDictionary ?? [:]
		let name = (info["CFBundleDisplayName"] as? String) ?? (info["CFBundleName"] as? String) ?? "SWORDApp"
		let app = name.replacingOccurrences(of: "[^A-Za-z0-9._-]", with: "", options: .regularExpression)
		let ver = info["CFBundleShortVersionString"] as? String ?? ""
		var sys = utsname()
		uname(&sys)
		var model = withUnsafePointer(to: &sys.machine) { $0.withMemoryRebound(to: CChar.self, capacity: 1) { String(cString: $0) } }
		if let simulated = ProcessInfo.processInfo.environment["SIMULATOR_MODEL_IDENTIFIER"] { model = simulated }
		return "\(app)/\(ver) (Apple \(model); \(UIDevice.current.systemName) \(UIDevice.current.systemVersion)) SWORD/\(engineVersion)"
	}

	@objc(initSWORD:)
	func initSWORD(command: CDVInvokedUrlCommand) {
		mgr = 0
		installMgr = 0
		disclaimerConfirmed = false
		mySWORDPlugin = nil
		VERSEKEY_BOOK = Int(org_crosswire_sword_SWModule_VERSEKEY_BOOK);
		VERSEKEY_CHAPTER = Int(org_crosswire_sword_SWModule_VERSEKEY_CHAPTER);
		VERSEKEY_VERSE = Int(org_crosswire_sword_SWModule_VERSEKEY_VERSE);
		VERSEKEY_TESTAMENT = Int(org_crosswire_sword_SWModule_VERSEKEY_TESTAMENT);
		VERSEKEY_OSISREF = Int(org_crosswire_sword_SWModule_VERSEKEY_OSISREF);
		VERSEKEY_CHAPTERMAX = Int(org_crosswire_sword_SWModule_VERSEKEY_CHAPTERMAX);
		VERSEKEY_VERSEMAX = Int(org_crosswire_sword_SWModule_VERSEKEY_VERSEMAX);
		VERSEKEY_BOOKNAME = Int(org_crosswire_sword_SWModule_VERSEKEY_BOOKNAME);
		VERSEKEY_SHORTTEXT = Int(org_crosswire_sword_SWModule_VERSEKEY_SHORTTEXT);
		VERSEKEY_BOOKABBREV = Int(org_crosswire_sword_SWModule_VERSEKEY_BOOKABBREV);
		VERSEKEY_OSISBOOKNAME = Int(org_crosswire_sword_SWModule_VERSEKEY_OSISBOOKNAME);
		VERSEKEY_COLLECTIONNAME = Int(org_crosswire_sword_SWModule_VERSEKEY_COLLECTIONNAME);

        LOG_ERROR = Int32(org_crosswire_sword_SWLog_LOG_ERROR);
        LOG_WARN = Int32(org_crosswire_sword_SWLog_LOG_WARN);
        LOG_INFO = Int32(org_crosswire_sword_SWLog_LOG_INFO);
        LOG_TIMEDINFO = Int32(org_crosswire_sword_SWLog_LOG_TIMEDINFO);
        LOG_DEBUG = Int32(org_crosswire_sword_SWLog_LOG_DEBUG);

        org_crosswire_sword_SWLog_setLogLevel(LOG_ERROR);
        
		org_crosswire_sword_StringMgr_setToUpper({ (text: Optional<UnsafePointer<Int8>>, maxBytes: u_long) in
			let lower = String(cString: text!)
			let upper = lower.uppercased()
			strncpy(UnsafeMutablePointer<Int8>(mutating: text), upper, Int(maxBytes));
			return UnsafeMutablePointer<Int8>(mutating: text)
		})

		initMgr()

		let libswordVersion = String(cString: org_crosswire_sword_SWMgr_version(mgr))
		debugPrint("libswordVersion: " + libswordVersion)
		if SWORD.userAgent == nil {
			let ua = buildUserAgent(engineVersion: libswordVersion)
			SWORD.userAgent = ua
			// the app's own XHRs say who is asking too
			if let wk = self.webView as? WKWebView {
				wk.evaluateJavaScript("navigator.userAgent") { result, _ in
					if let base = result as? String, !base.contains(ua) { wk.customUserAgent = base + " " + ua }
				}
			}
		}
        let info = [
            "version": libswordVersion,
            // what the page may offer on iOS (SWORD.features)
            // bibleSync: multicast needs com.apple.developer.networking.multicast,
            // which Apple granted the account on 2026-10-01 and Bishop's config.xml carries
            "features": [ "dataStorage": true, "dataFiles": true, "sharedStorage": false, "bibleSync": true ]
        ] as [AnyHashable : Any]
		self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: info), callbackId: command.callbackId)
	}


	func myToast(message: String) {
		let toastController: UIAlertController = UIAlertController(title: "", message: message, preferredStyle: .alert)
		self.viewController?.present(toastController, animated: true, completion: nil)
		DispatchQueue.main.asyncAfter(deadline: .now() + 5) {
			toastController.dismiss(animated: true, completion: nil)
		}
	}


	func initMgr() {
		if (mgr == 0) {
			let baseDir = (FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path)! + "/sword";
			mgr = org_crosswire_sword_SWMgr_newWithPath(baseDir)
            org_crosswire_sword_SWMgr_setGlobalOption(mgr, "Footnotes", "On")
            org_crosswire_sword_SWMgr_setGlobalOption(mgr, "Cross-references", "On")
debugPrint("initMgr, mgr: " + String(describing: mgr))
		}
	}


	func reinitMgr() {
		if (mgr != 0) {
			org_crosswire_sword_SWMgr_delete(mgr)
		}
		mgr = 0
		initMgr()
	}

    
    func reinitInstall() {
        if (installMgr != 0) {
            org_crosswire_sword_InstallMgr_delete(installMgr)
        }
        installMgr = 0
        initInstall()
    }
    
    func logError(message: String) {
        org_crosswire_sword_SWLog_logError(message)
    }
    func logDebug(message: String) {
        org_crosswire_sword_SWLog_logDebug(message)
    }
    func logWarning(message: String) {
        org_crosswire_sword_SWLog_logWarning(message)
    }
    func logInformation(message: String) {
        org_crosswire_sword_SWLog_logInformation(message)
    }
    func logTimedInformation(message: String) {
        org_crosswire_sword_SWLog_logTimedInformation(message)
    }

    
    func initInstall() {
    
        if (installMgr == 0) {
            logDebug(message: "initInstall: installMgr is null");
            let baseDir = (FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path)! + "/sword";
            // the engine's default accepts an unverified TLS peer; Android turns
            // that off (its JNI sets UnverifiedPeerAllowed false), and so do we
            _ = org_crosswire_sword_SWConfig_augmentConfig(baseDir + "/InstallMgr.conf", "[General]\nPassiveFTP=true\nUnverifiedPeerAllowed=false\n")
            installMgr = org_crosswire_sword_InstallMgr_new(baseDir, { (message: Optional<UnsafePointer<Int8>>, totalBytes: u_long, completedBytes: u_long) in
                let msg = String(cString: message!)
                if (msg == "update") {
                    let response = [
                        "status": "update",
                        "totalBytes": totalBytes,
                        "completedBytes": completedBytes
                        ] as [String : Any]
                    if (mySWORDPlugin != nil && mySWORDPlugin!.callbackID != "") {
                        let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: response)
                        result.setKeepCallbackAs(true)
                        mySWORDPlugin!.commandDelegate!.send(result, callbackId: mySWORDPlugin!.callbackID)
                    }
                }
                else {
                    let response = [
                        "status": "preStatus",
                        "totalBytes": totalBytes,
                        "completedBytes": completedBytes,
                        "message": msg
                        ] as [String : Any]
                    if (mySWORDPlugin != nil && mySWORDPlugin!.callbackID != "") {
                        let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: response)
                        result.setKeepCallbackAs(true)
                        mySWORDPlugin!.commandDelegate!.send(result, callbackId: mySWORDPlugin!.callbackID)
                    }
                }
            })
            if (disclaimerConfirmed) {
                org_crosswire_sword_InstallMgr_setUserDisclaimerConfirmed(installMgr)
            }
            logDebug(message: "initInstall: instantiated InstallMgr with baseDir: \(baseDir)");
        }
    }

	@objc(SWMgr_getModuleByName:)
	func SWMgr_getModuleByName(command: CDVInvokedUrlCommand) {

		initMgr();

		let modName = command.arguments[0] as? String ?? ""
        let module = org_crosswire_sword_SWMgr_getModuleByName(mgr, modName)

        if (module == 0) {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK), callbackId: command.callbackId)
            return
        }
        
        let name = org_crosswire_sword_SWModule_getName(module)
        let description = org_crosswire_sword_SWModule_getDescription(module)
        let category = org_crosswire_sword_SWModule_getCategory(module)

        var response = [
            "name": name == nil ? "" : String(cString: name!),
            "description": description == nil ? "" : String(cString: description!),
            "category": category == nil ? "" : String(cString: category!)
        ]
        let language = org_crosswire_sword_SWModule_getConfigEntry(module, "Lang")
        response["language"] =  language == nil ? "" : String(cString: language!)
        let direction = org_crosswire_sword_SWModule_getConfigEntry(module, "Direction")
        response["direction"] =  direction == nil ? "" : String(cString: direction!)
        let font = org_crosswire_sword_SWModule_getConfigEntry(module, "Font")
        response["font"] =  font == nil ? "" : String(cString: font!)
        let shortCopyright = org_crosswire_sword_SWModule_getConfigEntry(module, "ShortCopyright")
        response["shortCopyright"] =  shortCopyright == nil ? "" : String(cString: shortCopyright!)
        let cipherKey = org_crosswire_sword_SWModule_getConfigEntry(module, "CipherKey")
        if (cipherKey != nil) {
            response["cipherKey"] = String(cString: cipherKey!)
        }
        let shortPromo = org_crosswire_sword_SWModule_getConfigEntry(module, "ShortPromo")
        response["shortPromo"] = shortPromo == nil ? "" : String(cString: shortPromo!)


		self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: response), callbackId: command.callbackId)
	}



    @objc(SWMgr_addExtraConfig:)
    func SWMgr_addExtraConfig(command: CDVInvokedUrlCommand) {
        let blob = command.arguments[0] as? String ?? ""
        let baseDir = (FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path)! + "/sword";
        let confPath = baseDir + "/extraConfig.conf";
        let retVal = getStringArray(buffer: org_crosswire_sword_SWConfig_augmentConfig(confPath, blob))
	self.reinitMgr()
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
    }
    
    
    @objc(SWMgr_setExtraConfigValue:)
    func SWMgr_setExtraConfigValue(command: CDVInvokedUrlCommand) {
        let section = command.arguments[0] as? String ?? ""
        let key = command.arguments[1] as? String ?? ""
        let val = command.arguments[2] as? String ?? ""
        let baseDir = (FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path)! + "/sword";
        let confPath = baseDir + "/extraConfig.conf";
        org_crosswire_sword_SWConfig_setKeyValue(confPath, section, key, val)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWMgr_setExtraConfigValue"), callbackId: command.callbackId)
    }

    
    @objc(SWMgr_getExtraConfigValue:)
    func SWMgr_getExtraConfigValue(command: CDVInvokedUrlCommand) {
        let section = command.arguments[0] as? String ?? ""
        let key = command.arguments[1] as? String ?? ""
        let baseDir = (FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path)! + "/sword";
        let confPath = baseDir + "/extraConfig.conf";
        let keyVal = org_crosswire_sword_SWConfig_getKeyValue(confPath, section, key)
        let retVal = keyVal == nil ? nil : String(cString:keyVal!)
        self.commandDelegate!.send((retVal == nil ? CDVPluginResult(status: CDVCommandStatus_OK) : CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal!)), callbackId: command.callbackId)
    }
    
    
    @objc(SWMgr_getExtraConfigKeys:)
    func SWMgr_getExtraConfigKeys(command: CDVInvokedUrlCommand) {
        let section = command.arguments[0] as? String ?? ""
        let baseDir = (FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path)! + "/sword";
        let confPath = baseDir + "/extraConfig.conf";
        let retVal = getStringArray(buffer: org_crosswire_sword_SWConfig_getSectionKeys(confPath, section))
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
    }

    
	@objc(SWMgr_getExtraConfigSections:)
	func SWMgr_getExtraConfigSections(command: CDVInvokedUrlCommand) {
		let baseDir = (FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first?.path)! + "/sword";
		let confPath = baseDir + "/extraConfig.conf";
        let retVal = getStringArray(buffer: org_crosswire_sword_SWConfig_getSections(confPath))
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
  }


    @objc(SWMgr_getAvailableLocales:)
    func SWMgr_getAvailableLocales(command: CDVInvokedUrlCommand) {
	initMgr()
        let retVal = getStringArray(buffer: org_crosswire_sword_SWMgr_getAvailableLocales(mgr))
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
    }


    @objc(SWMgr_setDefaultLocale:)
    func SWMgr_setDefaultLocale(command: CDVInvokedUrlCommand) {
        initMgr()
        let localeName = command.arguments[0] as? String ?? ""
        org_crosswire_sword_SWMgr_setDefaultLocale(mgr, localeName)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWMgr_setDefaultLocale"), callbackId: command.callbackId)
    }
    

    @objc(SWMgr_translate:)
    func SWMgr_translate(command: CDVInvokedUrlCommand) {

        initMgr()

        let text = command.arguments[0] as? String ?? ""
        let localeName = command.arguments[1] as? String ?? ""

        let translated = org_crosswire_sword_SWMgr_translate(mgr, text, localeName == "null" || localeName == "" ? nil : localeName)
        let retVal = translated == nil ? nil : String(cString:translated!)
        self.commandDelegate!.send((retVal == nil ? CDVPluginResult(status: CDVCommandStatus_OK) : CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal!)), callbackId: command.callbackId)
    }
    

    @objc(SWMgr_getGlobalOptions:)
    func SWMgr_getGlobalOptions(command: CDVInvokedUrlCommand) {
        initMgr()
        let retVal = getStringArray(buffer: org_crosswire_sword_SWMgr_getGlobalOptions(mgr))
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
    }

    @objc(SWMgr_getGlobalOptionValues:)
    func SWMgr_getGlobalOptionValues(command: CDVInvokedUrlCommand) {
        initMgr()
        let option = command.arguments[0] as? String ?? ""
        let retVal = getStringArray(buffer: org_crosswire_sword_SWMgr_getGlobalOptionValues(mgr, option))
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
    }

    @objc(SWMgr_getGlobalOption:)
    func SWMgr_getGlobalOption(command: CDVInvokedUrlCommand) {
        initMgr()
        let option = command.arguments[0] as? String ?? ""
        let v = org_crosswire_sword_SWMgr_getGlobalOption(mgr, option)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: v == nil ? "" : String(cString:v!)), callbackId: command.callbackId)
    }

    @objc(SWMgr_getGlobalOptionTip:)
    func SWMgr_getGlobalOptionTip(command: CDVInvokedUrlCommand) {
        initMgr()
        let option = command.arguments[0] as? String ?? ""
        let v = org_crosswire_sword_SWMgr_getGlobalOptionTip(mgr, option)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: v == nil ? "" : String(cString:v!)), callbackId: command.callbackId)
    }

    @objc(SWMgr_getPrefixPath:)
    func SWMgr_getPrefixPath(command: CDVInvokedUrlCommand) {

        initMgr()

        let prefixPath = org_crosswire_sword_SWMgr_getPrefixPath(mgr)
        let retVal = prefixPath == nil ? nil : String(cString:prefixPath!)
        self.commandDelegate!.send((retVal == nil ? CDVPluginResult(status: CDVCommandStatus_OK) : CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal!)), callbackId: command.callbackId)
    }

    // --- study data: bookmarks and other per-user data, beside the module
    // library (the default), in a folder the user picks (iCloud Drive, On My
    // iPhone, ...), or as picked FILES: a provider such as Google Drive gives
    // other apps no folder in place, but does give files.  Picks are kept as
    // security-scoped bookmarks and opened for the life of the app.
    let DATA_STORAGE_KEY = "SWORDDataStorageBookmark"
    let DATA_FILES_KEY = "SWORDDataFileBookmarks"	// [file name: bookmark]
    let DATA_PROVIDER_KEY = "SWORDDataProvider"	// what the user chose: "iCloud Drive", "Google Drive", ...
    let STUDY_DATA_FOLDER = "sword-studydata"	// the app-neutral folder name (doc/bookmarks-format.md)
    var dataStorageURL: URL? = nil
    var dataFiles: [String: URL] = [:]
    var dataStorageLoaded = false
    // picked files appear to the page as this folder, holding bookmarks/
    let DATA_FILES_ROOT = "/sword-data/"
    // the document picker in flight: its callback id and what it is choosing
    var pendingPick: (callbackId: String, purpose: String)? = nil
    var pickProvider = ""	// the provider named by the page for the pick in flight

    // plugin file operations run one at a time, off the main thread: a
    // provider's file may have to come down before it can be read
    let fileQueue = DispatchQueue(label: "org.crosswire.sword.files")

    func resolveBookmark(_ data: Data) -> (URL, Data?)? {
        var stale = false
        guard let url = try? URL(resolvingBookmarkData: data, options: [], relativeTo: nil, bookmarkDataIsStale: &stale) else { return nil }
        // false for an item needing no grant (inside the app); usable either way
        _ = url.startAccessingSecurityScopedResource()
        return (url, stale ? (try? url.bookmarkData()) : nil)
    }

    func loadDataStorage() {
        if dataStorageLoaded { return }
        dataStorageLoaded = true
        if let data = UserDefaults.standard.data(forKey: DATA_STORAGE_KEY) {
            if let (url, fresh) = resolveBookmark(data) {
                dataStorageURL = url
                if let fresh = fresh { UserDefaults.standard.set(fresh, forKey: DATA_STORAGE_KEY) }
            }
            else { logError(message: "SWORD: the study-data folder could not be found; study data lives beside the library") }
        }
        if var saved = UserDefaults.standard.dictionary(forKey: DATA_FILES_KEY) as? [String: Data] {
            for (name, data) in saved {
                if let (url, fresh) = resolveBookmark(data) {
                    dataFiles[name] = url
                    if let fresh = fresh { saved[name] = fresh }
                }
                else { logError(message: "SWORD: the picked study-data file " + name + " could not be found") }
            }
            UserDefaults.standard.set(saved, forKey: DATA_FILES_KEY)
        }
    }

    func releaseDataStorage() {
        dataStorageURL?.stopAccessingSecurityScopedResource()
        dataStorageURL = nil
        for (_, url) in dataFiles { url.stopAccessingSecurityScopedResource() }
        dataFiles = [:]
        UserDefaults.standard.removeObject(forKey: DATA_STORAGE_KEY)
        UserDefaults.standard.removeObject(forKey: DATA_FILES_KEY)
        UserDefaults.standard.removeObject(forKey: DATA_PROVIDER_KEY)
    }

    var dataProvider: String { UserDefaults.standard.string(forKey: DATA_PROVIDER_KEY) ?? "" }

    // a picked folder is judged by its contents first, as on Android: one
    // holding bookmarks/ at its root is the study-data folder as it is (whatever
    // its name: a provider may report an old name for a while after a rename),
    // and so is one named sword-studydata (a folder just created to be it is
    // still empty); otherwise sword-studydata/ inside it, existing or made on
    // the first write
    func dataFolderPath() -> String? {
        guard let folder = dataStorageURL else { return nil }
        var isDir: ObjCBool = false
        if FileManager.default.fileExists(atPath: folder.appendingPathComponent("bookmarks").path, isDirectory: &isDir), isDir.boolValue { return folder.path }
        if folder.lastPathComponent == STUDY_DATA_FOLDER { return folder.path }
        return folder.appendingPathComponent(STUDY_DATA_FOLDER).path
    }

    // use (or, with nil, stop using) a folder as the study-data folder
    func setDataStorage(_ url: URL?, provider: String = "") {
        loadDataStorage()
        releaseDataStorage()
        guard let url = url else { return }
        _ = url.startAccessingSecurityScopedResource()
        if let data = try? url.bookmarkData() {
            UserDefaults.standard.set(data, forKey: DATA_STORAGE_KEY)
            UserDefaults.standard.set(provider, forKey: DATA_PROVIDER_KEY)
            dataStorageURL = url
        }
        else {
            url.stopAccessingSecurityScopedResource()
            logError(message: "SWORD: could not keep access to " + url.path)
        }
    }

    // use picked files (by name) as the study data's bookmarks/, in place of
    // whatever was chosen before, or (adding) alongside the files already picked
    func setDataFiles(_ urls: [URL], adding: Bool = false, provider: String = "") {
        loadDataStorage()
        var saved: [String: Data] = [:]
        if adding { saved = UserDefaults.standard.dictionary(forKey: DATA_FILES_KEY) as? [String: Data] ?? [:] }
        else { releaseDataStorage(); UserDefaults.standard.set(provider, forKey: DATA_PROVIDER_KEY) }
        for url in urls {
            _ = url.startAccessingSecurityScopedResource()
            if let data = try? url.bookmarkData() {
                saved[url.lastPathComponent] = data
                dataFiles[url.lastPathComponent] = url
            }
            else {
                url.stopAccessingSecurityScopedResource()
                logError(message: "SWORD: could not keep access to " + url.path)
            }
        }
        if !saved.isEmpty { UserDefaults.standard.set(saved, forKey: DATA_FILES_KEY) }
    }

    // "files:<provider>" while picked files are in use, else the folder's URL, else ""
    func dataStorageIdentifier() -> String {
        if !dataFiles.isEmpty { return "files:" + dataProvider }
        return dataStorageURL?.absoluteString ?? ""
    }

    // the chosen study-data folder, the picked files' virtual folder, else the library
    @objc(SWMgr_getDataPath:)
    func SWMgr_getDataPath(command: CDVInvokedUrlCommand) {
        initMgr()
        loadDataStorage()
        var retVal = ""
        if !dataFiles.isEmpty { retVal = DATA_FILES_ROOT }
        else if let folder = dataFolderPath() { retVal = folder }
        else {
            let prefixPath = org_crosswire_sword_SWMgr_getPrefixPath(mgr)
            retVal = prefixPath == nil ? "" : String(cString:prefixPath!)
        }
        if !retVal.hasSuffix("/") { retVal += "/" }
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
    }

    // the separately chosen study data, or "" if it lives beside the library
    @objc(SWMgr_getDataStorage:)
    func SWMgr_getDataStorage(command: CDVInvokedUrlCommand) {
        loadDataStorage()
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: dataStorageIdentifier()), callbackId: command.callbackId)
    }

    // the system folder picker; answers the chosen folder, or "" when cancelled
    @objc(SWMgr_chooseDataStorage:)
    func SWMgr_chooseDataStorage(command: CDVInvokedUrlCommand) {
        loadDataStorage()
        pickProvider = command.arguments.first as? String ?? ""
        presentPicker(command: command, purpose: "data", types: [UTType.folder], multiple: false, startingAt: dataStorageURL)
    }

    // the system picker over files, several at once (e.g. the .conf files in a
    // Google Drive bookmarks folder); answers "files:<provider>", or "" when cancelled
    @objc(SWMgr_chooseDataFiles:)
    func SWMgr_chooseDataFiles(command: CDVInvokedUrlCommand) {
        loadDataStorage()
        pickProvider = command.arguments.first as? String ?? ""
        presentPicker(command: command, purpose: "datafiles", types: [UTType.item], multiple: true, startingAt: dataFiles.values.first?.deletingLastPathComponent())
    }

    @objc(SWMgr_clearDataStorage:)
    func SWMgr_clearDataStorage(command: CDVInvokedUrlCommand) {
        loadDataStorage()
        releaseDataStorage()
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK), callbackId: command.callbackId)
    }

    func presentPicker(command: CDVInvokedUrlCommand, purpose: String, types: [UTType], multiple: Bool, startingAt: URL?) {
        if pendingPick != nil {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR, messageAs: "a choice is already in progress"), callbackId: command.callbackId)
            return
        }
        pendingPick = (command.callbackId, purpose)
        let picker = UIDocumentPickerViewController(forOpeningContentTypes: types, asCopy: false)
        picker.delegate = self
        picker.allowsMultipleSelection = multiple
        if let start = startingAt { picker.directoryURL = start }
        self.viewController?.present(picker, animated: true, completion: nil)
        let pending = CDVPluginResult(status: CDVCommandStatus_NO_RESULT)
        pending.setKeepCallbackAs(true)
        self.commandDelegate!.send(pending, callbackId: command.callbackId)
    }

    // --- pickers the file queue waits on: a provider which grants no folder
    // (e.g. Google Drive) still takes files the user saves there
    var pickCompletion: (([URL]?) -> Void)? = nil

    // present a picker on the main thread; the calling (background) thread
    // waits for its answer: the URLs, or nil when cancelled or busy
    func pickAndWait(_ make: @escaping () -> UIDocumentPickerViewController) -> [URL]? {
        let done = DispatchSemaphore(value: 0)
        var result: [URL]? = nil
        // a moment for any sheet just answered to finish going away
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.6) {
            if self.pendingPick != nil { done.signal(); return }
            self.pendingPick = ("", "internal")
            self.pickCompletion = { urls in result = urls; done.signal() }
            let picker = make()
            picker.delegate = self
            self.viewController?.present(picker, animated: true, completion: nil)
        }
        done.wait()
        return result
    }

    func noticeAndWait(_ title: String, _ message: String) {
        let done = DispatchSemaphore(value: 0)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.6) {
            let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: "OK", style: .default) { _ in done.signal() })
            self.viewController?.present(alert, animated: true, completion: nil)
        }
        done.wait()
    }

    // Files on a provider which grants no folder (e.g. Google Drive): the user
    // saves them there (an export), then opens them once.  The URL an export
    // answers is a placeholder the provider does not serve (measured: coordinated
    // reads fail, writes never upload), so only the opened URLs are kept.  nil if
    // either step is cancelled; whatever was saved then stays for a later pick.
    //   items: what to export (files, or one folder); names: the file names to
    //   open afterwards; where: what to tell the user to open
    func exportAndOpen(_ items: [URL], names: [String], where: String, near folder: URL?) -> [URL]? {
        guard let exported = pickAndWait({
            let p = UIDocumentPickerViewController(forExporting: items, asCopy: false)
            if let f = folder { p.directoryURL = f }
            return p
        }), !exported.isEmpty else { return nil }
        // the picker shows names without their extension unless the user asked for it
        let shown = names.map { ($0 as NSString).deletingPathExtension }
        noticeAndWait("Open what was saved", "Bishop needs to open the " + (names.count == 1 ? "file" : "files") + " it just saved: " + `where` + (names.count == 1 ? ", then tap " + shown[0] + "." : ", select " + shown.joined(separator: ", ") + ", then tap Open."))
        guard let opened = pickAndWait({
            let p = UIDocumentPickerViewController(forOpeningContentTypes: [UTType.item], asCopy: false)
            p.allowsMultipleSelection = names.count > 1
            p.directoryURL = folder ?? exported[0].deletingLastPathComponent()
            return p
        }) else { return nil }
        let wanted = opened.filter { names.contains($0.lastPathComponent) }
        return wanted.isEmpty ? nil : wanted
    }

    // first run on a provider without folder access: the bookmark files Bishop
    // has now (beside the library, or in the chosen folder) are saved there as
    // sword-studydata/bookmarks/*.conf in a folder the user picks, then opened,
    // and become the picked set.  Answers "files:<provider>", or "" if cancelled.
    @objc(SWMgr_exportDataFiles:)
    func SWMgr_exportDataFiles(command: CDVInvokedUrlCommand) {
        initMgr()
        loadDataStorage()
        let provider = command.arguments.first as? String ?? ""
        var source = ""
        if let folder = dataFolderPath() { source = folder }
        else if let prefixPath = org_crosswire_sword_SWMgr_getPrefixPath(mgr) { source = String(cString: prefixPath) }
        let bookmarksDir = (source as NSString).appendingPathComponent("bookmarks")
        fileQueue.async {
            var answer = ""
            let tmp = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent("export-\(Int(Date().timeIntervalSince1970))")
            let folder = tmp.appendingPathComponent(self.STUDY_DATA_FOLDER)
            let target = folder.appendingPathComponent("bookmarks")
            try? FileManager.default.createDirectory(at: target, withIntermediateDirectories: true)
            var names: [String] = []
            if self.dataFiles.isEmpty {
                for name in ((try? FileManager.default.contentsOfDirectory(atPath: bookmarksDir)) ?? []).sorted() where name.hasSuffix(".conf") {
                    var data: Data? = nil
                    try? self.coordinate((bookmarksDir as NSString).appendingPathComponent(name), writing: false) { u in data = try Data(contentsOf: u) }
                    if let d = data, (try? d.write(to: target.appendingPathComponent(name))) != nil { names.append(name) }
                }
            }
            if !names.isEmpty, let made = self.exportAndOpen([folder], names: names, where: "open " + self.STUDY_DATA_FOLDER + " \u{203A} bookmarks", near: nil) {
                self.setDataFiles(made, provider: provider)
                answer = self.dataStorageIdentifier()
            }
            try? FileManager.default.removeItem(at: tmp)
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: answer), callbackId: command.callbackId)
        }
    }

    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
        guard let pick = pendingPick else { return }
        pendingPick = nil
        if pick.purpose == "internal" {
            let done = pickCompletion
            pickCompletion = nil
            done?(urls)
            return
        }
        var answer = ""
        if pick.purpose == "data", let url = urls.first {
            setDataStorage(url, provider: pickProvider)
            answer = dataStorageIdentifier()
        }
        else if pick.purpose == "datafiles" && !urls.isEmpty {
            setDataFiles(urls, provider: pickProvider)
            answer = dataStorageIdentifier()
        }
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: answer), callbackId: pick.callbackId)
    }

    func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
        guard let pick = pendingPick else { return }
        pendingPick = nil
        if pick.purpose == "internal" {
            let done = pickCompletion
            pickCompletion = nil
            done?(nil)
            return
        }
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: ""), callbackId: pick.callbackId)
    }


    // a label for the chosen study data, as Android's "<provider> / <folder>":
    // "iCloud Drive / Bishop", or for picked files "Google Drive / 3 files"
    @objc(SWMgr_describeTree:)
    func SWMgr_describeTree(command: CDVInvokedUrlCommand) {
        let arg = command.arguments[0] as? String ?? ""
        loadDataStorage()
        var label = ""
        if arg.hasPrefix("files:") {
            let n = dataFiles.count
            label = (dataProvider.isEmpty ? "" : dataProvider + " \u{00B7} ") + (n == 1 ? "1 collection" : "\(n) collections")
        }
        else if let url = URL(string: arg) {
            let name = (try? url.resourceValues(forKeys: [.localizedNameKey]))?.localizedName ?? url.lastPathComponent
            let provider = dataProvider.isEmpty ? providerName(url) : dataProvider
            label = provider.isEmpty ? name : provider + " \u{00B7} " + name
        }
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: label), callbackId: command.callbackId)
    }

    // the provider holding a folder: "iCloud Drive", "On My iPhone", or a
    // third-party provider's (e.g. Google Drive's) own name for its storage
    func providerName(_ url: URL) -> String {
        let path = url.path
        if path.contains("/Mobile Documents/") { return "iCloud Drive" }
        if path.hasPrefix(FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0].path) {
            return UIDevice.current.userInterfaceIdiom == .pad ? "On My iPad" : "On My iPhone"
        }
        if path.contains("/File Provider Storage") {
            // walk up to the folder just below File Provider Storage
            var top = url
            while top.pathComponents.count > 1 && !top.deletingLastPathComponent().path.hasSuffix("/File Provider Storage") {
                top = top.deletingLastPathComponent()
            }
            let names = [url, top].map { (try? $0.resourceValues(forKeys: [.localizedNameKey]))?.localizedName ?? $0.lastPathComponent }
            if path.contains("com.google") || top.path.contains("com.google") { return "Google Drive" }
            return names[1]
        }
        return ""
    }

    func describeFolder(_ url: URL) -> String {
        let name = (try? url.resourceValues(forKeys: [.localizedNameKey]))?.localizedName ?? url.lastPathComponent
        let provider = providerName(url)
        return provider.isEmpty ? name : provider + " / " + name
    }

    // Plain file access, one operation at a time off the main thread.  A path
    // in a picked folder or file may belong to a File Provider that syncs it
    // (iCloud, Drive): NSFileCoordinator has the provider fetch its latest
    // version before a read (measured: an Android revision on Google Drive is
    // seen by the next coordinated read, while a plain read lags), and keeps a
    // write from interleaving with the provider's own.
    func coordinate(_ path: String, writing: Bool, options: NSFileCoordinator.WritingOptions = [], _ body: (URL) throws -> Void) throws {
        try coordinate(url: URL(fileURLWithPath: path), writing: writing, options: options, body)
    }

    func coordinate(url: URL, writing: Bool, options: NSFileCoordinator.WritingOptions = [], _ body: (URL) throws -> Void) throws {
        var coordinationError: NSError? = nil
        var bodyError: Error? = nil
        let coordinator = NSFileCoordinator(filePresenter: nil)
        if writing {
            coordinator.coordinate(writingItemAt: url, options: options, error: &coordinationError) { u in
                do { try body(u) } catch { bodyError = error }
            }
        }
        else {
            coordinator.coordinate(readingItemAt: url, options: [], error: &coordinationError) { u in
                do { try body(u) } catch { bodyError = error }
            }
        }
        if let e = coordinationError { throw e }
        if let e = bodyError { throw e }
    }

    // a provider's item is rewritten in place, so it keeps its identity there;
    // a temporary-and-rename makes it a new item, and other devices holding
    // the old one lose it
    func writeInPlace(_ url: URL, _ data: Data) throws {
        try coordinate(url: url, writing: true) { u in
            if FileManager.default.fileExists(atPath: u.path) {
                let handle = try FileHandle(forWritingTo: u)
                defer { try? handle.close() }
                try handle.truncate(atOffset: 0)
                try handle.write(contentsOf: data)
                try handle.synchronize()
            }
            else {
                try data.write(to: u)
            }
        }
    }

    // within the app's own container (the library, private study data)
    func isInAppContainer(_ path: String) -> Bool {
        let home = URL(fileURLWithPath: NSHomeDirectory()).resolvingSymlinksInPath().path
        let target = URL(fileURLWithPath: path).resolvingSymlinksInPath().path
        return target == home || target.hasPrefix(home + "/")
    }

    // a path in the picked files' virtual folder: nil if it is not one;
    // .some(nil) if it is, but names no picked file
    func pickedFile(_ path: String) -> URL?? {
        if !path.hasPrefix(DATA_FILES_ROOT) { return nil }
        loadDataStorage()
        let rest = String(path.dropFirst(DATA_FILES_ROOT.count))
        guard rest.hasPrefix("bookmarks/") else { return .some(nil) }
        return .some(dataFiles[String(rest.dropFirst("bookmarks/".count))])
    }

    @objc(SWMgr_readFile:)
    func SWMgr_readFile(command: CDVInvokedUrlCommand) {
        let path = command.arguments[0] as? String ?? ""
        fileQueue.async {
            var contents: String? = nil
            if let picked = self.pickedFile(path) {
                if let url = picked { try? self.coordinate(url: url, writing: false) { u in contents = try String(contentsOf: u, encoding: .utf8) } }
            }
            else { try? self.coordinate(path, writing: false) { u in contents = try String(contentsOf: u, encoding: .utf8) } }
            if let contents = contents {
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: contents), callbackId: command.callbackId)
            } else {
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR, messageAs: "couldn't read: " + path), callbackId: command.callbackId)
            }
        }
    }

    @objc(SWMgr_writeFile:)
    func SWMgr_writeFile(command: CDVInvokedUrlCommand) {
        let path = command.arguments[0] as? String ?? ""
        let contents = command.arguments[1] as? String ?? ""
        fileQueue.async {
            do {
                if let picked = self.pickedFile(path) {
                    if let url = picked { try self.writeInPlace(url, Data(contents.utf8)) }
                    else {
                        // a new file where no folder is granted: the user saves it
                        // beside the picked files, then opens it (user actions only
                        // create bookmark files, so a picker here is expected)
                        let name = (path as NSString).lastPathComponent
                        guard path.hasPrefix(self.DATA_FILES_ROOT + "bookmarks/"), !name.isEmpty else { throw CocoaError(.fileNoSuchFile) }
                        let tmp = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent("new-\(Int(Date().timeIntervalSince1970))")
                        try FileManager.default.createDirectory(at: tmp, withIntermediateDirectories: true)
                        defer { try? FileManager.default.removeItem(at: tmp) }
                        let local = tmp.appendingPathComponent(name)
                        try Data(contents.utf8).write(to: local)
                        // said first: a save screen from nowhere explains nothing, and
                        // a second file of the same name beside the first would split
                        // the collection (Google Drive allows two of one name)
                        self.noticeAndWait(name, "Bishop needs a bookmark file named " + name + ", which is not among the files you chose. If it is already in that folder, cancel the next screen and choose the files again, with it. Otherwise save it beside the others.")
                        guard let made = self.exportAndOpen([local], names: [name], where: "go to the folder you chose", near: self.dataFiles.values.first?.deletingLastPathComponent()),
                              let url = made.first(where: { $0.lastPathComponent == name }) else { throw CocoaError(.userCancelled) }
                        self.setDataFiles([url], adding: true)
                    }
                }
                else {
                    try FileManager.default.createDirectory(atPath: (path as NSString).deletingLastPathComponent, withIntermediateDirectories: true)
                    if self.isInAppContainer(path) {
                        try self.coordinate(path, writing: true, options: .forReplacing) { u in try contents.write(to: u, atomically: true, encoding: .utf8) }
                    }
                    else {
                        try self.writeInPlace(URL(fileURLWithPath: path), Data(contents.utf8))
                    }
                }
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK), callbackId: command.callbackId)
            } catch {
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR, messageAs: "couldn't write: " + path), callbackId: command.callbackId)
            }
        }
    }

    // names, with "/" on directories.  iCloud may show a file not yet on the
    // device as ".name.icloud": it is listed as "name" and asked for.  The
    // picked files' virtual folder lists the picked names under bookmarks/.
    @objc(SWMgr_listDir:)
    func SWMgr_listDir(command: CDVInvokedUrlCommand) {
        let path = command.arguments[0] as? String ?? ""
        fileQueue.async {
            var names: [String] = []
            if path.hasPrefix(self.DATA_FILES_ROOT) {
                self.loadDataStorage()
                let rest = String(path.dropFirst(self.DATA_FILES_ROOT.count))
                if rest.isEmpty || rest == "/" { names = ["bookmarks/"] }
                else if rest == "bookmarks" || rest == "bookmarks/" { names = self.dataFiles.keys.sorted() }
            }
            else {
                try? self.coordinate(path, writing: false) { dir in
                    for e in try FileManager.default.contentsOfDirectory(atPath: dir.path) {
                        let full = (dir.path as NSString).appendingPathComponent(e)
                        if e.hasPrefix(".") && e.hasSuffix(".icloud") && e.count > 8 {
                            try? FileManager.default.startDownloadingUbiquitousItem(at: URL(fileURLWithPath: full))
                            names.append(String(e.dropFirst().dropLast(7)))
                            continue
                        }
                        var isDir: ObjCBool = false
                        FileManager.default.fileExists(atPath: full, isDirectory: &isDir)
                        names.append(isDir.boolValue ? e + "/" : e)
                    }
                }
            }
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: names), callbackId: command.callbackId)
        }
    }

    @objc(SWMgr_removeFile:)
    func SWMgr_removeFile(command: CDVInvokedUrlCommand) {
        let path = command.arguments[0] as? String ?? ""
        fileQueue.async {
            // a picked file is not ours to delete
            if !path.hasPrefix(self.DATA_FILES_ROOT), (try? self.coordinate(path, writing: true, options: .forDeleting) { u in try FileManager.default.removeItem(at: u) }) != nil {
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK), callbackId: command.callbackId)
            } else {
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR, messageAs: "couldn't remove: " + path), callbackId: command.callbackId)
            }
        }
    }

    @objc(SWMgr_mkdir:)
    func SWMgr_mkdir(command: CDVInvokedUrlCommand) {
        let path = command.arguments[0] as? String ?? ""
        fileQueue.async {
            // the picked files' virtual folder always "exists"
            if path.hasPrefix(self.DATA_FILES_ROOT) || (try? FileManager.default.createDirectory(atPath: path, withIntermediateDirectories: true)) != nil {
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK), callbackId: command.callbackId)
            } else {
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR, messageAs: "couldn't create: " + path), callbackId: command.callbackId)
            }
        }
    }

    
    @objc(echo:)
    func echo(command: CDVInvokedUrlCommand) {
        let msg = command.arguments[0] as? String ?? ""
        myToast(message: msg)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: msg), callbackId: command.callbackId)
    }
    
    
    // HTTPUtils.METHOD_GET = 0, METHOD_POST = 1 (SWORD.js), as on Android.
    // The answer is the body, on an HTTP error status too (callers want a
    // JSON error message), or no message when the request could not be made.
    @objc(HTTPUtils_makeRequest:)
    func HTTPUtils_makeRequest(command: CDVInvokedUrlCommand) {
        var url = command.arguments[0] as? String ?? ""
        let postData = command.arguments[1] as? String ?? ""
        let method = command.arguments[2] as? Int ?? 1
        let isGet = (method == 0)

        if isGet && !postData.isEmpty {
            if !url.contains("?") {
                if !postData.hasPrefix("?") { url += "?" }
            }
            else if !url.hasSuffix("&") && !postData.hasPrefix("&") { url += "&" }
            url += postData
        }
        guard let target = URL(string: url) else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK), callbackId: command.callbackId)
            return
        }
        var request = URLRequest(url: target)
        request.httpMethod = isGet ? "GET" : "POST"
        request.setValue("*/*", forHTTPHeaderField: "Accept")
        request.setValue("application/x-www-form-urlencoded", forHTTPHeaderField: "Content-Type")
        if let ua = SWORD.userAgent { request.setValue(ua, forHTTPHeaderField: "User-Agent") }
        if !isGet { request.httpBody = postData.data(using: .utf8) }
        let session = URLSession.shared
        session.dataTask(with: request) {data, response, err in
            let body = data.flatMap { String(data: $0, encoding: String.Encoding.utf8) }
            let pluginResult = body == nil ? CDVPluginResult(status: CDVCommandStatus_OK) : CDVPluginResult(status: CDVCommandStatus_OK, messageAs: body!)
            pluginResult.setKeepCallbackAs(false)
            self.commandDelegate!.send(pluginResult, callbackId: command.callbackId)
        }.resume()

        let pluginResult = CDVPluginResult(status: CDVCommandStatus_NO_RESULT)
        pluginResult.setKeepCallbackAs(true)
        self.commandDelegate!.send(pluginResult, callbackId: command.callbackId)
  }
    
    
    @objc(SWModule_getBookNames:)
    func SWModule_getBookNames(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        var retVal = [[AnyHashable : Any]]()
        org_crosswire_sword_SWModule_begin(mod)
        while (org_crosswire_sword_SWModule_popError(mod) == 0) {
            let vkInfo = getStringArray(buffer: org_crosswire_sword_SWModule_getKeyChildren(mod));
            let bookInfo = [
                "name": vkInfo[VERSEKEY_BOOKNAME],
                "abbrev": vkInfo[VERSEKEY_BOOKABBREV],
                "osisName": vkInfo[VERSEKEY_OSISBOOKNAME],
                "chapterMax": Int(vkInfo[VERSEKEY_CHAPTERMAX])!,
            ] as [AnyHashable : Any]
            retVal.append(bookInfo)

            org_crosswire_sword_SWModule_setKeyText(mod, "+book")
        }
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
    }
    
    
    @objc(SWModule_getVerseKey:)
    func SWModule_getVerseKey(command: CDVInvokedUrlCommand) {
        initMgr()
        let module = getModule(command: command)
        if (module != 0) {
            let retVal = getVerseKey(keyChildren: getStringArray(buffer: org_crosswire_sword_SWModule_getKeyChildren(module)))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }


    @objc(SWModule_begin:)
    func SWModule_begin(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            org_crosswire_sword_SWModule_begin(mod)
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWModule_begin"), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }

    
    @objc(SWModule_previous:)
    func SWModule_previous(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            org_crosswire_sword_SWModule_previous(mod)
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWModule_previous"), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    
    @objc(SWModule_next:)
    func SWModule_next(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            org_crosswire_sword_SWModule_next(mod)
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWModule_next"), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    
    @objc(SWModule_popError:)
    func SWModule_popError(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let error = Int(org_crosswire_sword_SWModule_popError(mod))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: error), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    
    @objc(SWModule_getEntryAttribute:)
    func SWModule_getEntryAttribute(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let retVal = getStringArray(buffer: org_crosswire_sword_SWModule_getEntryAttribute(mod, command.arguments[1] as? String ?? "", command.arguments[2] as? String ?? "", command.arguments[3] as? String ?? "", (command.arguments[4] as? Bool ?? false) ? 1 : 0))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    @objc(SWModule_parseKeyList:)
    func SWModule_parseKeyList(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let retVal = getStringArray(buffer: org_crosswire_sword_SWModule_parseKeyList(mod, command.arguments[1] as? String ?? ""))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    @objc(SWModule_getConfigEntry:)
    func SWModule_getConfigEntry(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command, remoteSourceArgNumber: 2)
        if (mod != 0) {
            let val = org_crosswire_sword_SWModule_getConfigEntry(mod, command.arguments[1] as? String ?? "")
            let retVal = val == nil ? nil : String(cString: val!)
            self.commandDelegate!.send((retVal == nil ? CDVPluginResult(status: CDVCommandStatus_OK) : CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal!)), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    
    @objc(SWModule_getKeyChildren:)
    func SWModule_getKeyChildren(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let retVal = getStringArray(buffer: org_crosswire_sword_SWModule_getKeyChildren(mod))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    
    @objc(SWModule_getRenderHeader:)
    func SWModule_getRenderHeader(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let header = String(cString: org_crosswire_sword_SWModule_getRenderHeader(mod))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: header), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    // the system share sheet with the verse's text and reference, as Android's chooser
    func shareVerse(module: Int, callbackId: String) {
        let verseKey = getStringArray(buffer: org_crosswire_sword_SWModule_getKeyChildren(module))
        let modName = String(cString: org_crosswire_sword_SWModule_getName(module))
        let verseText = String(cString: org_crosswire_sword_SWModule_stripText(module))
        let ref = verseKey.count > VERSEKEY_SHORTTEXT ? verseKey[VERSEKEY_SHORTTEXT] : String(cString: org_crosswire_sword_SWModule_getKeyText(module))
        let sheet = UIActivityViewController(activityItems: [verseText + " --" + ref + " (" + modName + ")"], applicationActivities: nil)
        sheet.completionWithItemsHandler = { _, completed, _, _ in
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: completed), callbackId: callbackId)
        }
        if let pop = sheet.popoverPresentationController, let view = self.viewController?.view {	// iPad: anchored under the top bar
            pop.sourceView = view
            pop.sourceRect = CGRect(x: view.bounds.midX, y: 60, width: 1, height: 1)
        }
        self.viewController?.present(sheet, animated: true, completion: nil)
    }

    @objc(SWModule_sendText:)
    func SWModule_sendText(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            shareVerse(module: mod, callbackId: command.callbackId)
            let result = CDVPluginResult(status: CDVCommandStatus_NO_RESULT)
            result.setKeepCallbackAs(true)
            self.commandDelegate!.send(result, callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }

    @objc(SWMgr_setGlobalOption:)
    func SWMgr_setGlobalOption(command: CDVInvokedUrlCommand) {
        initMgr()
        let option = command.arguments[0] as? String ?? ""
        let value = command.arguments[1] as? String ?? ""
        org_crosswire_sword_SWMgr_setGlobalOption(mgr, option, value)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWModule_setGlobalOption"), callbackId: command.callbackId)
    }
    
    @objc(SWModule_getRenderText:)
    func SWModule_getRenderText(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let retVal = String(cString: org_crosswire_sword_SWModule_renderText(mod))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    @objc(SWModule_getStripText:)
    func SWModule_getStripText(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let retVal = String(cString: org_crosswire_sword_SWModule_stripText(mod))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    @objc(SWModule_getRawEntry:)
    func SWModule_getRawEntry(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let retVal = String(cString: org_crosswire_sword_SWModule_getRawEntry(mod))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }

    
    @objc(SWMgr_startBibleSync:)
    func SWMgr_startBibleSync(command: CDVInvokedUrlCommand) {
        initMgr()
        let appName = command.arguments[0] as? String ?? ""
        let userName = command.arguments[1] as? String ?? ""
        let passphrase = command.arguments[2] as? String ?? ""
            bibleSyncCallbackID = command.callbackId
            DispatchQueue.global().async {
                mySWORDPlugin = self
                org_crosswire_sword_SWMgr_startBibleSync(self.mgr, appName, userName, passphrase, { (cmd : Int8, str1: Optional<UnsafePointer<Int8>>, str2: Optional<UnsafePointer<Int8>>) in
                    // a nav message carries no second string (flatapi passes 0)
                    let response1 = str1 == nil ? "" : String(cString: str1!)
                    let response2 = str2 == nil ? "" : String(cString: str2!)
                    // (CChar("N") is Int8's failable parse of the string "N", i.e. nil,
                    // which equals no command: every message was dropped that way)
                    if (mySWORDPlugin != nil && mySWORDPlugin!.bibleSyncCallbackID != "") {
			if (cmd == CChar(UInt8(ascii: "N"))) {
				var retVal = [String:Any]()
				retVal["cmd"]     = "nav";
				retVal["osisRef"] = response1;
				let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal)
				result.setKeepCallbackAs(true)
				mySWORDPlugin!.commandDelegate!.send(result, callbackId: mySWORDPlugin!.bibleSyncCallbackID)
			}
			else if (cmd == CChar(UInt8(ascii: "C"))	) {
				var retVal = [String:Any]()
				retVal["cmd"]     = "chat";
				retVal["user"]    = response1;
				retVal["message"] = response2;
				let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal)
				result.setKeepCallbackAs(true)
				mySWORDPlugin!.commandDelegate!.send(result, callbackId: mySWORDPlugin!.bibleSyncCallbackID)
			}
			else if (cmd == CChar(UInt8(ascii: "I"))) {
				// the interface address BibleSync is on
				var retVal = [String:Any]()
				retVal["cmd"]       = "info";
				retVal["interface"] = response1;
				let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal)
				result.setKeepCallbackAs(true)
				mySWORDPlugin!.commandDelegate!.send(result, callbackId: mySWORDPlugin!.bibleSyncCallbackID)
			}
			else if (cmd == CChar(UInt8(ascii: "A")) || cmd == CChar(UInt8(ascii: "S")) || cmd == CChar(UInt8(ascii: "D"))) {
				// a peer announced itself, became a speaker we listen to, or went quiet
				var retVal = [String:Any]()
				retVal["cmd"]   = "peer";
				retVal["state"] = cmd == CChar(UInt8(ascii: "A")) ? "announce" : (cmd == CChar(UInt8(ascii: "S")) ? "speaker" : "gone");
				retVal["who"]   = response1;	// user @ address ('gone': the uuid)
				retVal["what"]  = response2;	// app version; device; uuid
				let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal)
				result.setKeepCallbackAs(true)
				mySWORDPlugin!.commandDelegate!.send(result, callbackId: mySWORDPlugin!.bibleSyncCallbackID)
			}
			else if (cmd == CChar(UInt8(ascii: "E"))) {
				// the library stopped itself: what it said, and its detail
				var retVal = [String:Any]()
				retVal["cmd"]     = "error";
				retVal["message"] = response1;
				retVal["detail"]  = response2;
				let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal)
				result.setKeepCallbackAs(true)
				mySWORDPlugin!.commandDelegate!.send(result, callbackId: mySWORDPlugin!.bibleSyncCallbackID)
			}
                    }
                });
                
                self.bibleSyncCallbackID = ""
                
                let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWMgr_startBibleSync finished")
                result.setKeepCallbackAs(false)
                self.commandDelegate!.send(result, callbackId: command.callbackId)
            }
            
            let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWMgr_startBibleSync")
            result.setKeepCallbackAs(true)
            self.commandDelegate!.send(result, callbackId: command.callbackId)
    }
    
    @objc(SWMgr_stopBibleSync:)
    func SWMgr_stopBibleSync(command: CDVInvokedUrlCommand) {
        initMgr()
        org_crosswire_sword_SWMgr_stopBibleSync(mgr)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWMgr_stopBibleSync"), callbackId: command.callbackId)
    }
    
    @objc(SWMgr_sendBibleSyncMessage:)
    func SWMgr_sendBibleSyncMessage(command: CDVInvokedUrlCommand) {
        initMgr()
        let osisRef = command.arguments[0] as? String ?? ""
        org_crosswire_sword_SWMgr_sendBibleSyncMessage(mgr, osisRef)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWMgr_sendBibleSyncMessage"), callbackId: command.callbackId)
    }
    
    
    @objc(SWModule_getRenderChapter:)
    func SWModule_getRenderChapter(command: CDVInvokedUrlCommand) {
        initMgr()
        
//        DispatchQueue.global().async {
            self.initMgr()
            let masterMod = self.getModule(command: command, nameArgNumber: 0)
            let mod = self.getModule(command: command, nameArgNumber: 1)
            if (masterMod != 0 && mod != 0) {
                let r = self.renderChapter(masterMod: masterMod, mod: mod)
                self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: r), callbackId: command.callbackId)
            }
//        }
/*
        var pluginResult = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWModule_getRenderChapter")
        pluginResult.setKeepCallbackAs(true)
        self.commandDelegate!.send(pluginResult, callbackId: command.callbackId)
*/
    }

    func renderChapter(masterMod: Int, mod: Int) -> [[String: Any]] {
        let saveMasterKey = String(cString: org_crosswire_sword_SWModule_getKeyText(masterMod))
        let saveKey = String(cString: org_crosswire_sword_SWModule_getKeyText(mod))
        var r = [[String: Any]]()
        let currentKey = getStringArray(buffer: org_crosswire_sword_SWModule_getKeyChildren(masterMod))
        // not a verse key (a dictionary or book as master): nothing to render by chapter
        if currentKey.count <= max(VERSEKEY_BOOKABBREV, VERSEKEY_CHAPTER, VERSEKEY_BOOK, VERSEKEY_OSISREF) { return r }
        let book = currentKey[VERSEKEY_BOOKABBREV]
        let chapter = currentKey[VERSEKEY_CHAPTER]
        org_crosswire_sword_SWModule_setKeyText(masterMod, book + "." + chapter + ".1")
        var verseKey = getStringArray(buffer: org_crosswire_sword_SWModule_getKeyChildren(masterMod))
        while (org_crosswire_sword_SWModule_popError(masterMod) == 0
            && currentKey[VERSEKEY_BOOK] == verseKey[VERSEKEY_BOOK]
            && currentKey[VERSEKEY_CHAPTER] == verseKey[VERSEKEY_CHAPTER]
            ) {
                org_crosswire_sword_SWModule_setKeyText(mod, verseKey[VERSEKEY_OSISREF])
                let error = org_crosswire_sword_SWModule_popError(mod)
                var v = [String:Any]()
                if (error == 0) {
                    v["verse"] = getVerseKey(keyChildren: getStringArray(buffer: org_crosswire_sword_SWModule_getKeyChildren(mod)))
                    v["text"] = String(cString: org_crosswire_sword_SWModule_renderText(mod))
                    var preVerse = ""
                    for i in getStringArray(buffer: org_crosswire_sword_SWModule_getEntryAttribute(mod, "Heading", "Preverse", "", 1)) {
                        preVerse += i
                    }
                    v["preVerse"] = preVerse
                }
                else {
                    
                }
                r.append(v)
                org_crosswire_sword_SWModule_next(masterMod)
                verseKey = getStringArray(buffer: org_crosswire_sword_SWModule_getKeyChildren(masterMod))
        }
        org_crosswire_sword_SWModule_setKeyText(masterMod, saveMasterKey)
        org_crosswire_sword_SWModule_setKeyText(mod, saveKey)
        
        return r
        
    }
    
    func getVerseKey(keyChildren:[String]) -> [String:Any] {
        var retVal = [String:Any]()
        if (keyChildren.count > 9) {
            retVal["testament"]    = Int(keyChildren[VERSEKEY_TESTAMENT]);
            retVal["book"]         = Int(keyChildren[VERSEKEY_BOOK]);
            retVal["chapter"]      = Int(keyChildren[VERSEKEY_CHAPTER]);
            retVal["verse"]        = Int(keyChildren[VERSEKEY_VERSE]);
            retVal["chapterMax"]   = Int(keyChildren[VERSEKEY_CHAPTERMAX]);
            retVal["verseMax"]     = Int(keyChildren[VERSEKEY_VERSEMAX]);
            retVal["bookName"]     = keyChildren[VERSEKEY_BOOKNAME];
            retVal["osisRef"]      = keyChildren[VERSEKEY_OSISREF];
            retVal["shortText"]    = keyChildren[VERSEKEY_SHORTTEXT];
            retVal["bookAbbrev"]   = keyChildren[VERSEKEY_BOOKABBREV];
            retVal["osisBookName"] = keyChildren[VERSEKEY_OSISBOOKNAME];
        }
        return retVal;
    }

    @objc(SWModule_terminateSearch:)
    func SWModule_terminateSearch(command: CDVInvokedUrlCommand) {
        let module = getModule(command: command)
        if (module != 0) {
            org_crosswire_sword_SWModule_terminateSearch(module)
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWModule_terminateSearch"), callbackId: command.callbackId)
        }
    }
    
    @objc(SWModule_search:)
    func SWModule_search(command: CDVInvokedUrlCommand) {
        initMgr()
        let mod = getModule(command: command)
        if (mod != 0) {
            let expression = command.arguments[1] as? String ?? ""
            let searchType = command.arguments[2] as? Int32 ?? 0
            let flags = command.arguments[3] as? Int ?? 0
            let scope = command.arguments.count < 5 ? nil : command.arguments[4] as? String ?? nil
            callbackID = command.callbackId
            DispatchQueue.global().async {
                mySWORDPlugin = self
                let buffer = org_crosswire_sword_SWModule_search(mod, expression, searchType, flags, scope, { (percent: Int32) in
                    let response = [
                        "status": "update",
                        "percent": percent
                        ] as [String : Any]
                    if (mySWORDPlugin != nil && mySWORDPlugin!.callbackID != "") {
                        let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: response)
                        result.setKeepCallbackAs(true)
                        mySWORDPlugin!.commandDelegate!.send(result, callbackId: mySWORDPlugin!.callbackID)
                    }
                });
                
                self.callbackID = ""
                var response = [
                    "status": "complete",
                    "percent": 100
                ] as [String : Any]

                var b = buffer
                var count = 0
                while let i = b?.pointee {
                    if i.key == nil {
                        break
                    }
                    count = count + 1
                    b = b?.advanced(by: 1)
                }
                let searchResults = UnsafeBufferPointer<org_crosswire_sword_SearchHit>(start: buffer, count: count);
                var results = [[String:Any]]()
                for i in searchResults {
                    let sr = [
                        "key": String(cString: i.key),
                        "score": Int(i.score)
                    ] as [String : Any]
                    results.append(sr)
                }
                response["results"] = results
                
                let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: response)
                result.setKeepCallbackAs(false)
                self.commandDelegate!.send(result, callbackId: command.callbackId)
            }
            
            let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWModule_search")
            result.setKeepCallbackAs(true)
            self.commandDelegate!.send(result, callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR), callbackId: command.callbackId)
        }
    }
    
    
    func getModule(command: CDVInvokedUrlCommand, nameArgNumber: Int = 0, remoteSourceArgNumber: Int = -1) -> Int {
        initMgr()
        let modName = command.arguments[nameArgNumber] as? String ?? ""
        let sourceName = remoteSourceArgNumber == -1 ? "" : command.arguments[remoteSourceArgNumber] as? String ?? ""
        var module = 0;
        if (sourceName != "") {
            logDebug(message: "remoteSourceName: \(sourceName)");
            initInstall()
            module = org_crosswire_sword_InstallMgr_getRemoteModuleByName(installMgr, sourceName, modName)
        }
        else {
            module = org_crosswire_sword_SWMgr_getModuleByName(mgr, modName)
        }
        if (module == 0) {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR, messageAs: "couldn't find module \(modName)"), callbackId: command.callbackId)
        }
        return module
    }
    
    
    @objc(SWModule_getKeyText:)
    func SWModule_getKeyText(command: CDVInvokedUrlCommand) {
        let module = getModule(command: command)
        if (module != 0) {
            let keyText = org_crosswire_sword_SWModule_getKeyText(module)
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: keyText == nil ? "" : String(cString: keyText!)), callbackId: command.callbackId)
        }
    }


    @objc(SWModule_setKeyText:)
    func SWModule_setKeyText(command: CDVInvokedUrlCommand) {
        let module = getModule(command: command)
        let keyText = command.arguments[1] as? String ?? ""
        if (module != 0) {
            let error = Int(org_crosswire_sword_SWModule_setKeyText(module, keyText))
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: error), callbackId: command.callbackId)
        }
    }


    @objc(SWModule_setRawEntry:)
    func SWModule_setRawEntry(command: CDVInvokedUrlCommand) {
        let module = getModule(command: command)
        let entryText = command.arguments[1] as? String ?? ""
        if (module != 0) {
            org_crosswire_sword_SWModule_setRawEntry(module, entryText)
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWModule_setEntryText"), callbackId: command.callbackId)
        }
    }
    
    
    @objc(InstallMgr_uninstallModule:)
    func InstallMgr_uninstallModule(command: CDVInvokedUrlCommand) {
        initInstall()
        initMgr()
        let retVal = org_crosswire_sword_InstallMgr_uninstallModule(installMgr, mgr, command.arguments[0] as? String ?? "")
        if (retVal == 0) {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: retVal), callbackId: command.callbackId)
        }
        else {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_ERROR, messageAs: retVal), callbackId: command.callbackId)
        }
    }
    
    var callbackID:String = ""
    var bibleSyncCallbackID:String = ""
    @objc(InstallMgr_remoteInstallModule:)
    func InstallMgr_remoteInstallModule(command: CDVInvokedUrlCommand) {
        initInstall()
        initMgr()
        let repo = command.arguments[0] as? String ?? ""
        let modName = command.arguments[1] as? String ?? ""
        callbackID = command.callbackId
        DispatchQueue.global().async {
            mySWORDPlugin = self
            org_crosswire_sword_InstallMgr_remoteInstallModule(self.installMgr, self.mgr, repo, modName)
            
            self.reinitMgr()
            self.callbackID = ""
            let response = [
                "status": "complete",
                "totalBytes": 0,
                "completedBytes": 0,
                "message": "Complete"
                ] as [String : Any]
            let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: response)
            result.setKeepCallbackAs(false)
            self.commandDelegate!.send(result, callbackId: command.callbackId)
        }

        let result = CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "InstallMgr_remoteInstallModule")
        result.setKeepCallbackAs(true)
        self.commandDelegate!.send(result, callbackId: command.callbackId)
    }

    
    @objc(InstallMgr_getRemoteModInfoList:)
    func InstallMgr_getRemoteModInfoList(command: CDVInvokedUrlCommand) {
        initInstall()
        initMgr()
        let buffer = org_crosswire_sword_InstallMgr_getRemoteModInfoList(installMgr, mgr, command.arguments[0] as? String ?? "")
        var count = 0
        var b = buffer
        while let i = b?.pointee {
            if (i.name == nil) {
                break
            }
            count = count + 1
            b = b?.advanced(by: 1)
        }
        let modInfoList = Array(UnsafeBufferPointer<org_crosswire_sword_ModInfo>(start: buffer, count: count));
        var mods = [[AnyHashable : Any]]()
        for i in modInfoList {
            var modInfo = [
                "name": String(cString: i.name),
                "description": String(cString: i.description),
                "category": String(cString: i.category),
                "language": String(cString: i.language),
                "delta": String(cString: i.delta),
                "version": String(cString: i.version),
                "features": getStringArray(buffer: i.features)
                ] as [AnyHashable : Any]

            if (i.cipherKey != nil) {
                modInfo["cipherKey"] = String(cString: i.cipherKey)
            }
            mods.append(modInfo)
        }
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: mods), callbackId: command.callbackId)
    }
    
    
    @objc(InstallMgr_getRemoteModuleByName:)
    func InstallMgr_getRemoteModuleByName(command: CDVInvokedUrlCommand) {
        
        initInstall()
        
        let modSource = command.arguments[0] as? String ?? ""
        let modName   = command.arguments[1] as? String ?? ""
        let module = org_crosswire_sword_InstallMgr_getRemoteModuleByName(installMgr, modSource, modName)
        
        if (module == 0) {
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK), callbackId: command.callbackId)
            return
        }
        
        let name = org_crosswire_sword_SWModule_getName(module)
        let description = org_crosswire_sword_SWModule_getDescription(module)
        let category = org_crosswire_sword_SWModule_getCategory(module)
        
        var response = [
            "name": name == nil ? "" : String(cString: name!),
            "description": description == nil ? "" : String(cString: description!),
            "category": category == nil ? "" : String(cString: category!),
            "remoteSourceName": modSource
        ]
        let language = org_crosswire_sword_SWModule_getConfigEntry(module, "Lang")
        response["language"] =  language == nil ? "" : String(cString: language!)
        let direction = org_crosswire_sword_SWModule_getConfigEntry(module, "Direction")
        response["direction"] =  direction == nil ? "" : String(cString: direction!)
        let font = org_crosswire_sword_SWModule_getConfigEntry(module, "Font")
        response["font"] =  font == nil ? "" : String(cString: font!)
        let shortCopyright = org_crosswire_sword_SWModule_getConfigEntry(module, "ShortCopyright")
        response["shortCopyright"] =  shortCopyright == nil ? "" : String(cString: shortCopyright!)
        let cipherKey = org_crosswire_sword_SWModule_getConfigEntry(module, "CipherKey")
        if (cipherKey != nil) {
            response["cipherKey"] = String(cString: cipherKey!)
        }
        let shortPromo = org_crosswire_sword_SWModule_getConfigEntry(module, "ShortPromo")
        response["shortPromo"] = shortPromo == nil ? "" : String(cString: shortPromo!)
        
        
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: response), callbackId: command.callbackId)
    }

    @objc(InstallMgr_refreshRemoteSource:)
    func InstallMgr_refreshRemoteSource(command: CDVInvokedUrlCommand) {
        initInstall()
        DispatchQueue.global().async {
            mySWORDPlugin = self
            self.callbackID = ""
            org_crosswire_sword_InstallMgr_refreshRemoteSource(self.installMgr, command.arguments[0] as? String ?? "")
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "InstallMgr_refreshRemoteSource"), callbackId: command.callbackId)
        }
    }

    
    func getStringArray(buffer: UnsafeMutablePointer<UnsafePointer<Int8>?>!) -> [String] {
        var sources = [String]()
        var b = buffer
        while let i = b?.pointee {
            sources.append(String(cString: i))
            b = b?.advanced(by: 1)
        }
        return sources
    }
    
    
    @objc(InstallMgr_getRemoteSources:)
    func InstallMgr_getRemoteSources(command: CDVInvokedUrlCommand) {
        initInstall()
        let sources = getStringArray(buffer: org_crosswire_sword_InstallMgr_getRemoteSources(installMgr))
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: sources), callbackId: command.callbackId)
    }

    
    @objc(InstallMgr_syncConfig:)
        func InstallMgr_syncConfig(command: CDVInvokedUrlCommand) {

        initInstall()
        DispatchQueue.global().async {
            self.callbackID = ""
            mySWORDPlugin = self
            org_crosswire_sword_InstallMgr_syncConfig(self.installMgr)
            self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "InstallMgr_syncConfig"), callbackId: command.callbackId)
        }
    }
    
    
    @objc(InstallMgr_setUserDisclaimerConfirmed:)
    func InstallMgr_setUserDisclaimerConfirmed(command: CDVInvokedUrlCommand) {
        initInstall()
        org_crosswire_sword_InstallMgr_setUserDisclaimerConfirmed(installMgr)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "InstallMgr_setUserDisclaimerConfirmed"), callbackId: command.callbackId)
    }
    

    @objc(SWMgr_setJavascript:)
    func SWMgr_setJavascript(command: CDVInvokedUrlCommand) {
        initMgr()
        org_crosswire_sword_SWMgr_setJavascript(mgr, command.arguments[0] as? Bool ?? true ? 1 : 0)
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: "SWMgr_setJavascript"), callbackId: command.callbackId)
    }
    
    @objc(SWMgr_getModInfoList:)
    func SWMgr_getModInfoList(command: CDVInvokedUrlCommand) {
        initMgr()
        let buffer = org_crosswire_sword_SWMgr_getModInfoList(mgr)
        var b = buffer
        var count = 0
        while let i = b?.pointee {
            if i.name == nil {
                break
            }
            b = b?.advanced(by: 1)
            count = count + 1
        }
        let modInfoList = Array(UnsafeBufferPointer<org_crosswire_sword_ModInfo>(start: buffer, count: count));
        
        var mods = [[AnyHashable : Any]]()
        for i in modInfoList {
            var modInfo = [
                "name": String(cString: i.name),
                "description": String(cString: i.description),
                "category": String(cString: i.category),
                "language": String(cString: i.language),
                "delta": i.delta == nil ? "" : String(cString: i.delta),
                "version": i.version == nil ? "" : String(cString: i.version),
                "features": getStringArray(buffer: i.features)
            ] as [AnyHashable : Any]

            if (i.cipherKey != nil) {
                modInfo["cipherKey"] = String(cString: i.cipherKey)
            }
            mods.append(modInfo)

        }
        self.commandDelegate!.send(CDVPluginResult(status: CDVCommandStatus_OK, messageAs: mods), callbackId: command.callbackId)
    }
}
