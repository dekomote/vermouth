import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: settingsPage

    // Store logins live in the store tabs, so these ask the main window to open them.
    signal loginRequested(string store)
    title: i18n("Settings")

    readonly property string localProtonDir: {
        settingsManager.localProtonDir;
        return protonScanner.localProtonPath();
    }

    function storageChanges() {
        var defs = [
            {
                "kind": "assets",
                "label": i18n("Assets"),
                "field": assetsDirField,
                "oldPath": protonScanner.localAssetsPath(),
                "def": protonScanner.defaultAssetsPath()
            },
            {
                "kind": "prefixes",
                "label": i18n("Prefixes"),
                "field": prefixDirField,
                "oldPath": protonScanner.prefixBasePath(),
                "def": protonScanner.defaultPrefixBasePath()
            },
            {
                "kind": "protons",
                "label": i18n("Local Proton"),
                "field": localProtonDirField,
                "oldPath": protonScanner.localProtonPath(),
                "def": protonScanner.defaultLocalProtonPath()
            },
            {
                "kind": "wines",
                "label": i18n("Local Wine"),
                "field": localWineDirField,
                "oldPath": wineScanner.localWinePath(),
                "def": wineScanner.defaultLocalWinePath()
            }
        ];
        var list = [];
        for (var i = 0; i < defs.length; i++) {
            var typed = settingsManager.cleanDir(defs[i].field.text);
            var newPath = typed !== "" ? typed : defs[i].def;
            if (newPath !== defs[i].oldPath)
                list.push({
                    "kind": defs[i].kind,
                    "label": defs[i].label,
                    "typed": typed,
                    "oldPath": defs[i].oldPath,
                    "newPath": newPath
                });
        }
        return list;
    }

    function applyStorageDir(change) {
        if (change.kind === "assets")
            settingsManager.setAssetsDir(change.typed);
        else if (change.kind === "protons")
            settingsManager.setLocalProtonDir(change.typed);
        else if (change.kind === "wines")
            settingsManager.setLocalWineDir(change.typed);
        else if (change.kind === "prefixes")
            settingsManager.setDefaultPrefixDir(change.typed);
    }

    function save() {
        settingsManager.setUmuPath(umuPathField.text);
        var changes = storageChanges();
        if (changes.length > 0) {
            moveDialog.errorText = "";
            moveDialog.changes = changes;
            moveDialog.open();
        }
        settingsManager.setDefaultGamePrefix(gamePrefixField.text);
        settingsManager.setDefaultWinePrefix(winePrefixField.text);
        settingsManager.setGamepadFullscreenButton(gamepadFullscreenCombo.currentValue);
        settingsManager.setSteamGridDbApiKey(steamGridDbKeyField.text);
        settingsManager.setRommServerUrl(rommUrlField.text.trim().replace(/\/+$/, ""));
        settingsManager.setRommApiKey(rommApiKeyField.text);
        settingsManager.setRetroarchPath(retroarchPathField.text);
        settingsManager.setRomCacheDir(romCacheDirField.text);
        settingsManager.setGogEnabled(gogEnabledSwitch.checked);
        settingsManager.setGogCacheDir(gogCacheDirField.text);
        settingsManager.setGogInstallDir(gogInstallDirField.text);
        settingsManager.setEpicEnabled(epicEnabledSwitch.checked);
        settingsManager.setEpicInstallDir(epicInstallDirField.text);
        defaultRuntimePicker.saveToSettings();
        var vars = [];
        for (var i = 0; i < envModel.count; i++) {
            var k = envModel.get(i).key.trim();
            var v = envModel.get(i).value.trim();
            if (k !== "")
                vars.push(k + "=" + v);
        }
        settingsManager.setGlobalEnvVars(vars);
        settingsManager.setGlobalLaunchOption("proton", launchOptsProtonField.text.trim());
        settingsManager.setGlobalLaunchOption("wine", launchOptsWineField.text.trim());
        settingsManager.setGlobalLaunchOption("native", launchOptsNativeField.text.trim());
        settingsManager.setGlobalLaunchOption("uzdoom", launchOptsUzdoomField.text.trim());
        settingsManager.setGlobalLaunchOption("dosbox", launchOptsDosboxField.text.trim());
        settingsManager.setGlobalLaunchOption("steam", launchOptsSteamField.text.trim());
        if (gpuManager.available && defaultGpuCombo.currentIndex >= 0)
            settingsManager.setDefaultGpu(defaultGpuCombo.value);
    }

    function load() {
        umuPathField.text = settingsManager.umuPath;
        prefixDirField.text = settingsManager.defaultPrefixDir;
        assetsDirField.text = settingsManager.assetsDir;
        localProtonDirField.text = settingsManager.localProtonDir;
        localWineDirField.text = settingsManager.localWineDir;
        gamePrefixField.text = settingsManager.defaultGamePrefix;
        winePrefixField.text = settingsManager.defaultWinePrefix;
        for (var gi = 0; gi < gamepadFullscreenCombo.model.length; gi++) {
            if (gamepadFullscreenCombo.model[gi].value === settingsManager.gamepadFullscreenButton) {
                gamepadFullscreenCombo.currentIndex = gi;
                break;
            }
        }
        steamGridDbKeyField.text = settingsManager.steamGridDbApiKey;
        rommUrlField.text = settingsManager.rommServerUrl;
        rommApiKeyField.text = settingsManager.rommApiKey;
        retroarchPathField.text = settingsManager.retroarchPath;
        romCacheDirField.text = settingsManager.romCacheDir;
        gogEnabledSwitch.checked = settingsManager.gogEnabled;
        gogCacheDirField.text = settingsManager.gogCacheDir;
        gogInstallDirField.text = settingsManager.gogInstallDir;
        epicEnabledSwitch.checked = settingsManager.epicEnabled;
        epicInstallDirField.text = settingsManager.epicInstallDir;
        pathsModel.clear();
        var paths = settingsManager.extraProtonPaths;
        for (var i = 0; i < paths.length; i++) {
            pathsModel.append({
                "path": paths[i]
            });
        }
        envModel.clear();
        var vars = settingsManager.globalEnvVars;
        for (var j = 0; j < vars.length; j++) {
            var sep = vars[j].indexOf("=");
            envModel.append({
                "key": sep > 0 ? vars[j].substring(0, sep) : vars[j],
                "value": sep > 0 ? vars[j].substring(sep + 1) : ""
            });
        }
        launchOptsProtonField.text = settingsManager.globalLaunchOptions["proton"] || "";
        launchOptsWineField.text = settingsManager.globalLaunchOptions["wine"] || "";
        launchOptsNativeField.text = settingsManager.globalLaunchOptions["native"] || "";
        launchOptsUzdoomField.text = settingsManager.globalLaunchOptions["uzdoom"] || "";
        launchOptsDosboxField.text = settingsManager.globalLaunchOptions["dosbox"] || "";
        launchOptsSteamField.text = settingsManager.globalLaunchOptions["steam"] || "";
        defaultGpuCombo.load(settingsManager.defaultGpu);
        defaultRuntimePicker.reset();
    }

    ListModel {
        id: pathsModel
    }

    ListModel {
        id: envModel
    }

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing

        RuntimePicker {
            id: defaultRuntimePicker
            Layout.fillWidth: true
            sectionLabel: i18n("Default Runtime")
            protonWineOnly: true
        }

        Kirigami.FormLayout {
            twinFormLayouts: defaultRuntimePicker.formLayout

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("umu-launcher")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("umu-launcher runs Proton through the Steam Runtime (pressure-vessel), which significantly improves game compatibility - especially for games with video cutscenes or anti-cheat. Strongly recommended.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            RowLayout {
                Layout.fillWidth: true

                Kirigami.FormData.label: i18n("umu-run path:")
                QQC2.TextField {
                    id: umuPathField
                    Layout.fillWidth: true
                    placeholderText: i18n("Auto-detect (umu-run in PATH)")
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: umuFilePicker.open()
                }
                QQC2.ToolButton {
                    icon.name: "folder-download-symbolic"
                    enabled: !umuDownloader.busy
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: umuDownloader.statusText ? umuDownloader.statusText : i18n("Download latest umu-launcher")
                    onClicked: umuDownloader.downloadLatest()
                }
            }

            DownloaderProgress {
                Layout.fillWidth: true
                Kirigami.FormData.label: ""
                visible: umuDownloader.busy
                progress: umuDownloader.progress
                statusText: umuDownloader.statusText
            }

            Connections {
                target: settingsManager
                function onUmuPathChanged() {
                    umuPathField.text = settingsManager.umuPath;
                }
            }

            Connections {
                target: umuDownloader
                function onFinished(binPath) {
                    if (umuPathField.text === "")
                        umuPathField.text = binPath;
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Prefixes")
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Default Prefix Parent Folder:")
                QQC2.TextField {
                    id: prefixDirField
                    Layout.fillWidth: true
                    placeholderText: protonScanner.prefixBasePath()
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: prefixDirFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("This is the folder where Vermouth stores all the created prefixes by default.")
                    icon.name: "help-about-symbolic"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Assets Folder:")
                QQC2.TextField {
                    id: assetsDirField
                    Layout.fillWidth: true
                    placeholderText: protonScanner.defaultAssetsPath()
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: assetsDirFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Where Vermouth keeps game art, extracted icons, and the DOSBox and UZDOOM it downloads. Leave empty for the default.")
                    icon.name: "help-about-symbolic"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Local Proton Folder:")
                QQC2.TextField {
                    id: localProtonDirField
                    Layout.fillWidth: true
                    placeholderText: protonScanner.defaultLocalProtonPath()
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: localProtonDirFolderDialog.open()
                }
                QQC2.ToolButton {
                    id: steamProtonButton
                    readonly property var steamPaths: protonScanner.steamCompatToolsPaths()
                    visible: steamPaths.length > 0
                    icon.name: "steam"
                    QQC2.ToolTip.text: i18n("Use Steam's compatibilitytools.d folder")
                    QQC2.ToolTip.visible: hovered
                    onClicked: {
                        if (steamPaths.length === 1)
                            localProtonDirField.text = steamPaths[0];
                        else
                            steamProtonMenu.popup(steamProtonButton, 0, steamProtonButton.height);
                    }

                    QQC2.Menu {
                        id: steamProtonMenu
                        Repeater {
                            model: steamProtonButton.steamPaths
                            delegate: QQC2.MenuItem {
                                required property string modelData
                                text: modelData
                                onTriggered: localProtonDirField.text = modelData
                            }
                        }
                    }
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Where Vermouth downloads Proton builds and umu-launcher, and the folder it scans for them. Leave empty for the default.")
                    icon.name: "help-about-symbolic"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Local Wine Folder:")
                QQC2.TextField {
                    id: localWineDirField
                    Layout.fillWidth: true
                    placeholderText: wineScanner.defaultLocalWinePath()
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: localWineDirFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Where Vermouth downloads Wine builds, and the folder it scans for them. Leave empty for the default.")
                    icon.name: "help-about-symbolic"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Default Proton Prefix:")
                QQC2.TextField {
                    id: gamePrefixField
                    Layout.fillWidth: true
                    placeholderText: i18n("Auto-generate per Proton game")
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: gamePrefixFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Set this if you want all Proton games to share a single prefix (e.g. one Proton environment for everything). Leave empty to auto-generate a separate prefix per game. You can still use separate prefix per game, but you have to set it explicitly.")
                    icon.name: "help-about-symbolic"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Default Wine Prefix:")
                QQC2.TextField {
                    id: winePrefixField
                    Layout.fillWidth: true
                    placeholderText: i18n("Auto-generate per Wine game")
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: winePrefixFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Set this if you want all Wine games to share a single prefix. Leave empty to auto-generate a separate Wine prefix per game under the 'wines/' subfolder.")
                    icon.name: "help-about-symbolic"
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("SteamGridDB")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("SteamGridDB API key is required to download game art. You can get a free key from steamgriddb.com/profile/preferences/api")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            QQC2.TextField {
                id: steamGridDbKeyField
                Kirigami.FormData.label: i18n("API Key:")
                Layout.fillWidth: true
                echoMode: TextInput.PasswordEchoOnEdit
                placeholderText: i18n("Paste your SteamGridDB API key")
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Auto-download Art:")
                QQC2.Switch {
                    id: autoDownloadSwitch
                    checked: settingsManager.autoDownloadArt
                    onToggled: settingsManager.setAutoDownloadArt(checked)
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Automatically download icon, grid, hero and logo when adding a game.")
                    icon.name: "help-about-symbolic"
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Steam Shortcuts")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("Adds games to your Steam library as non-Steam shortcuts by rewriting Steam's shortcuts.vdf.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Add to Steam:")
                QQC2.Switch {
                    checked: settingsManager.steamShortcutsEnabled
                    onToggled: settingsManager.setSteamShortcutsEnabled(checked)
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Experimental: this rewrites your Steam shortcuts.vdf and might mess up your existing Steam shortcuts. Make sure Steam is closed and back up shortcuts.vdf before using it.")
                    icon.name: "help-about-symbolic"
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("RomM")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("RomM is a self-hosted ROM manager. Set your server URL and API key to browse and launch ROMs via RetroArch.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            QQC2.TextField {
                id: rommUrlField
                Kirigami.FormData.label: i18n("Server URL:")
                Layout.fillWidth: true
                placeholderText: "http://your-romm-server:3000"
            }

            QQC2.TextField {
                id: rommApiKeyField
                Kirigami.FormData.label: i18n("API Key:")
                Layout.fillWidth: true
                echoMode: TextInput.PasswordEchoOnEdit
                placeholderText: i18n("Your RomM API key")
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("ROM Cache Folder:")
                QQC2.TextField {
                    id: romCacheDirField
                    Layout.fillWidth: true
                    placeholderText: i18n("Default: AppData/romm")
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: romCacheFolderDialog.open()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("GOG")
            }

            QQC2.Switch {
                id: gogEnabledSwitch
                Kirigami.FormData.label: i18n("GOG Library:")
                text: i18n("Enable")
            }

            QQC2.Label {
                visible: gogEnabledSwitch.checked
                Kirigami.FormData.label: ""
                text: i18n("Log in from the GOG Library tab to browse and install your owned GOG games.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            RowLayout {
                visible: gogEnabledSwitch.checked
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Account:")
                QQC2.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: gogClient.authenticated ? (gogClient.username !== "" ? i18n("Logged in as %1", gogClient.username) : i18n("Logged in")) : i18n("Not logged in")
                }
                QQC2.Button {
                    text: i18n("Log in")
                    icon.name: "system-log-in-symbolic"
                    visible: !gogClient.authenticated
                    onClicked: settingsPage.loginRequested("gog")
                }
                QQC2.Button {
                    text: i18n("Log out")
                    icon.name: "system-log-out-symbolic"
                    enabled: gogClient.authenticated
                    onClicked: gogClient.logout()
                }
            }

            RowLayout {
                visible: gogEnabledSwitch.checked
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Download Folder:")
                QQC2.TextField {
                    id: gogCacheDirField
                    Layout.fillWidth: true
                    placeholderText: i18n("Default: AppData/gog")
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: gogCacheFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Where GOG installers are downloaded before installation.")
                    icon.name: "help-about-symbolic"
                }
            }

            RowLayout {
                visible: gogEnabledSwitch.checked
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Install Folder:")
                QQC2.TextField {
                    id: gogInstallDirField
                    Layout.fillWidth: true
                    placeholderText: i18n("Default: ~/GOG Games")
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: gogInstallFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Where GOG games are installed.")
                    icon.name: "help-about-symbolic"
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Epic Games")
            }

            QQC2.Switch {
                id: epicEnabledSwitch
                Kirigami.FormData.label: i18n("Epic Library:")
                text: i18n("Enable")
            }

            QQC2.Label {
                visible: epicEnabledSwitch.checked
                Kirigami.FormData.label: ""
                text: epicClient.available ? i18n("Log in from the Epic Library tab to browse and install your Epic games. Uses the legendary command line tool. Installed games are added to your game list.") : i18n("The legendary command line tool was not found. Install it (for example with pip or your package manager) and press Check again.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: epicClient.available ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.negativeTextColor
            }

            RowLayout {
                visible: epicEnabledSwitch.checked
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Account:")
                QQC2.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: epicClient.authenticated ? (epicClient.username !== "" ? i18n("Logged in as %1", epicClient.username) : i18n("Logged in")) : i18n("Not logged in")
                }
                QQC2.Button {
                    text: i18n("Check again")
                    icon.name: "view-refresh-symbolic"
                    visible: !epicClient.available
                    onClicked: epicClient.detect()
                }
                QQC2.Button {
                    text: i18n("Log in")
                    icon.name: "system-log-in-symbolic"
                    visible: !epicClient.authenticated && epicClient.available
                    onClicked: settingsPage.loginRequested("epic")
                }
                QQC2.Button {
                    text: i18n("Log out")
                    icon.name: "system-log-out-symbolic"
                    enabled: epicClient.authenticated
                    onClicked: epicClient.logout()
                }
            }

            RowLayout {
                visible: epicEnabledSwitch.checked
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Install Folder:")
                QQC2.TextField {
                    id: epicInstallDirField
                    Layout.fillWidth: true
                    placeholderText: i18n("Default: ~/Epic Games")
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: epicInstallFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Where Epic games are installed. Wine prefixes use your default prefix folders.")
                    icon.name: "help-about-symbolic"
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("RetroArch")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("RetroArch is used to launch ROMs. It will be auto-detected if installed in your PATH or as a Flatpak.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            FlatpakHostHint {
                Kirigami.FormData.label: ""
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("RetroArch binary:")
                QQC2.TextField {
                    id: retroarchPathField
                    Layout.fillWidth: true
                    placeholderText: i18n("Auto-detect")
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: retroarchFilePicker.open()
                }
                QQC2.ToolButton {
                    icon.name: "view-refresh-symbolic"
                    QQC2.ToolTip.text: i18n("Auto-detect RetroArch")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: {
                        var detected = launcher.detectRetroarchPath();
                        if (detected !== "")
                            retroarchPathField.text = detected;
                    }
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Extra Proton Scan Paths")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("Folders to scan for Proton installations, in addition to Steam and local paths.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            ColumnLayout {
                Kirigami.FormData.label: i18n("Scan Folders:")
                Layout.fillWidth: true
                Repeater {
                    model: pathsModel
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        QQC2.TextField {
                            text: model.path
                            Layout.fillWidth: true
                            readOnly: true
                        }
                        QQC2.ToolButton {
                            icon.name: "list-remove"
                            onClicked: {
                                settingsManager.removeExtraProtonPath(index);
                                pathsModel.remove(index);
                            }
                        }
                    }
                }

                QQC2.Button {
                    text: i18n("Add Path...")
                    icon.name: "list-add-symbolic"
                    onClicked: protonPathFolderDialog.open()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Global Environment Variables")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("Applied to every game. Per-game launch options can override these.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            ColumnLayout {
                Kirigami.FormData.label: i18n("Variables:")
                Layout.fillWidth: true

                Repeater {
                    model: envModel
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        QQC2.TextField {
                            placeholderText: i18n("KEY")
                            text: model.key
                            implicitWidth: Kirigami.Units.gridUnit * 9
                            onTextChanged: envModel.setProperty(index, "key", text)
                        }
                        QQC2.Label {
                            text: "="
                        }
                        QQC2.TextField {
                            placeholderText: i18n("value")
                            text: model.value
                            Layout.fillWidth: true
                            onTextChanged: envModel.setProperty(index, "value", text)
                        }
                        QQC2.ToolButton {
                            icon.name: "list-remove"
                            onClicked: envModel.remove(index)
                        }
                    }
                }

                RowLayout {
                    QQC2.Button {
                        text: i18n("Add Variable")
                        icon.name: "list-add-symbolic"
                        onClicked: envModel.append({
                            "key": "",
                            "value": ""
                        })
                    }
                    EnvPresetButton {
                        targetModel: envModel
                    }
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Global Launch Options")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("Applied to every game that uses the runtime. A game's own launch options replace these. Use %command% to place the game's command, e.g. gamemoderun %command%.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            QQC2.TextField {
                id: launchOptsProtonField
                Kirigami.FormData.label: i18n("Proton:")
                Layout.fillWidth: true
                placeholderText: i18n("e.g. gamemoderun %command%")
            }

            QQC2.TextField {
                id: launchOptsWineField
                Kirigami.FormData.label: i18n("Wine:")
                Layout.fillWidth: true
                placeholderText: i18n("e.g. gamemoderun %command%")
            }

            QQC2.TextField {
                id: launchOptsNativeField
                Kirigami.FormData.label: i18n("Native:")
                Layout.fillWidth: true
                placeholderText: i18n("e.g. gamemoderun %command%")
            }

            QQC2.TextField {
                id: launchOptsUzdoomField
                Kirigami.FormData.label: i18n("UZDOOM:")
                Layout.fillWidth: true
                placeholderText: i18n("e.g. -skill 3")
            }

            QQC2.TextField {
                id: launchOptsDosboxField
                Kirigami.FormData.label: i18n("DOSBox Staging:")
                Layout.fillWidth: true
                placeholderText: i18n("e.g. --fullscreen")
            }

            QQC2.TextField {
                id: launchOptsSteamField
                Kirigami.FormData.label: i18n("Steam:")
                Layout.fillWidth: true
                placeholderText: i18n("e.g. gamemoderun %command%")
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Graphics")
                visible: gpuManager.available
            }

            GpuPicker {
                id: defaultGpuCombo
                Kirigami.FormData.label: i18n("Default GPU:")
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                QQC2.ToolTip.text: i18n("The GPU games run on unless a game picks its own. Not used for Steam and RetroArch games.")
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Appearance")
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Alt. Grid background:")
                QQC2.Switch {
                    checked: settingsManager.gridAltBackground
                    onToggled: settingsManager.setGridAltBackground(checked)
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Use the alternative background color for the grid view. It's lighter on Breeze light and darker on Breeze dark.")
                    icon.name: "help-about-symbolic"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Show scroll bar:")
                QQC2.Switch {
                    checked: settingsManager.showScrollBar
                    onToggled: settingsManager.setShowScrollBar(checked)
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Show a vertical scroll bar on the game grids.")
                    icon.name: "help-about-symbolic"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Theme:")
                QQC2.ComboBox {
                    id: themeCombo
                    model: colorSchemeSwitcher.schemeNames
                    Component.onCompleted: currentIndex = colorSchemeSwitcher.indexForSchemeId(settingsManager.themeId)
                    onActivated: settingsManager.setThemeId(colorSchemeSwitcher.schemeIdAt(currentIndex))
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Dark Theme:")
                QQC2.ComboBox {
                    id: bigScreenThemeCombo
                    model: colorSchemeSwitcher.schemeNames
                    Component.onCompleted: currentIndex = colorSchemeSwitcher.indexForSchemeId(settingsManager.bigScreenThemeId)
                    onActivated: settingsManager.setBigScreenThemeId(colorSchemeSwitcher.schemeIdAt(currentIndex))
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Used in Lights Out and Big Screen mode.")
                    icon.name: "help-about-symbolic"
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Tips")
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Show Tips:")
                QQC2.Switch {
                    checked: settingsManager.showTips
                    onToggled: settingsManager.setShowTips(checked)
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Show helpful prompts like the Steam import suggestion on first launch.")
                    icon.name: "help-about-symbolic"
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Gamepad")
            }

            QQC2.ComboBox {
                id: gamepadFullscreenCombo
                Kirigami.FormData.label: i18n("Fullscreen Toggle:")
                model: [
                    {
                        text: i18n("Guide (default)"),
                        value: "guide"
                    },
                    {
                        text: i18n("Select + L2"),
                        value: "selectl2"
                    },
                    {
                        text: i18n("L3 + R3"),
                        value: "l3r3"
                    }
                ]
                textRole: "text"
                valueRole: "value"
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Backup")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("Save your settings or your game list to a file, or restore them from one. Settings backups include API keys and logins, so keep them private. Restoring first keeps a copy of the current file next to it with a .bak ending.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Settings:")
                QQC2.Button {
                    text: i18n("Back Up…")
                    icon.name: "document-save-symbolic"
                    onClicked: backupSettingsDialog.open()
                }
                QQC2.Button {
                    text: i18n("Restore…")
                    icon.name: "document-open-symbolic"
                    onClicked: restoreSettingsDialog.open()
                }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Game list:")
                QQC2.Button {
                    text: i18n("Back Up…")
                    icon.name: "document-save-symbolic"
                    onClicked: backupGamesDialog.open()
                }
                QQC2.Button {
                    text: i18n("Restore…")
                    icon.name: "document-open-symbolic"
                    onClicked: restoreGamesDialog.open()
                }
            }
        }
    }

    function backupDone(error, message) {
        if (error !== "")
            applicationWindow().showPassiveNotification(error, 5000);
        else
            applicationWindow().showPassiveNotification(message, 3000);
    }

    function today() {
        return Qt.formatDate(new Date(), "yyyy-MM-dd");
    }

    FileDialog {
        id: backupSettingsDialog
        title: i18n("Back Up Settings")
        fileMode: FileDialog.SaveFile
        currentFolder: "file://" + protonScanner.homePath()
        currentFile: "file://" + protonScanner.homePath() + "/vermouth-settings-" + settingsPage.today() + ".conf"
        nameFilters: [i18n("Settings backup (*.conf)"), i18n("All files (*)")]
        onAccepted: settingsPage.backupDone(backupManager.backupSettings(selectedFile.toString()), i18n("Settings backed up"))
    }

    FileDialog {
        id: restoreSettingsDialog
        title: i18n("Restore Settings")
        fileMode: FileDialog.OpenFile
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Settings backup (*.conf)"), i18n("All files (*)")]
        onAccepted: {
            var error = backupManager.restoreSettings(selectedFile.toString());
            if (error !== "")
                settingsPage.backupDone(error, "");
            else
                restartDialog.open();
        }
    }

    FileDialog {
        id: backupGamesDialog
        title: i18n("Back Up Game List")
        fileMode: FileDialog.SaveFile
        currentFolder: "file://" + protonScanner.homePath()
        currentFile: "file://" + protonScanner.homePath() + "/vermouth-games-" + settingsPage.today() + ".json"
        nameFilters: [i18n("Game list backup (*.json)"), i18n("All files (*)")]
        onAccepted: settingsPage.backupDone(backupManager.backupGames(selectedFile.toString()), i18n("Game list backed up"))
    }

    FileDialog {
        id: restoreGamesDialog
        title: i18n("Restore Game List")
        fileMode: FileDialog.OpenFile
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Game list backup (*.json)"), i18n("All files (*)")]
        onAccepted: settingsPage.backupDone(backupManager.restoreGames(selectedFile.toString()), i18n("Game list restored"))
    }

    Kirigami.PromptDialog {
        id: restartDialog
        title: i18n("Settings Restored")
        subtitle: i18n("Vermouth has to restart to apply the restored settings. It will close now; start it again afterwards.")
        standardButtons: Kirigami.Dialog.Ok
        onAccepted: Qt.quit()
    }

    FileDialog {
        id: umuFilePicker
        title: i18n("Select umu-run binary")
        currentFolder: umuPathField.text !== "" ? "file://" + umuPathField.text.substring(0, umuPathField.text.lastIndexOf("/")) : "file://" + protonScanner.homePath()
        onAccepted: umuPathField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }

    StorageMoveDialog {
        id: moveDialog
        onApplyDirect: function (change) {
            settingsPage.applyStorageDir(change);
        }
        onCancelled: settingsPage.load()
    }

    FolderDialog {
        id: assetsDirFolderDialog
        title: i18n("Select Assets Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: assetsDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: localProtonDirFolderDialog
        title: i18n("Select Local Proton Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: localProtonDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: localWineDirFolderDialog
        title: i18n("Select Local Wine Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: localWineDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: prefixDirFolderDialog
        title: i18n("Select Default Prefix Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: prefixDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: gamePrefixFolderDialog
        title: i18n("Select Default App/Game Prefix")
        currentFolder: "file://" + protonScanner.prefixBasePath()
        onAccepted: gamePrefixField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: winePrefixFolderDialog
        title: i18n("Select Default Wine Prefix")
        currentFolder: "file://" + protonScanner.winePrefixBasePath()
        onAccepted: winePrefixField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: protonPathFolderDialog
        title: i18n("Select Proton Scan Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: {
            var path = decodeURIComponent(selectedFolder.toString().replace("file://", ""));
            settingsManager.addExtraProtonPath(path);
            pathsModel.append({
                "path": path
            });
        }
    }

    FileDialog {
        id: retroarchFilePicker
        title: i18n("Select RetroArch binary")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: retroarchPathField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }

    FolderDialog {
        id: romCacheFolderDialog
        title: i18n("Select ROM Cache Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: romCacheDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: gogCacheFolderDialog
        title: i18n("Select GOG Download Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: gogCacheDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: gogInstallFolderDialog
        title: i18n("Select GOG Install Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: gogInstallDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: epicInstallFolderDialog
        title: i18n("Select Epic Install Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: epicInstallDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }
}
