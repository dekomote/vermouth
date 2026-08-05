import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: editMode ? i18n("Edit Game") : i18n("Add Game")
    preferredWidth: Kirigami.Units.gridUnit * 35
    padding: Kirigami.Units.largeSpacing
    bottomPadding: 30
    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: [
        Kirigami.Action {
            text: i18n("OK")
            icon.name: "dialog-ok"
            onTriggered: {
                if (dialog.validate()) {
                    dialog.doSave();
                    dialog.close();
                }
            }
        },
        Kirigami.Action {
            text: i18n("Cancel")
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    property bool editMode: false
    property int editIndex: -1
    property string prefixBasePath
    property string autoDownloadTargetId: ""
    property bool pendingAutoDownload: false
    property bool autoDownloadingInDialog: false
    property bool pendingInstallerRun: false
    property string autoDownloadStatus: ""
    property string installerExePath: ""
    property bool installerRunning: false
    property int installerPid: 0
    property string installerPrefixPath: ""

    function openForNew() {
        editMode = false;
        editIndex = -1;
        nameField.text = "";
        exeField.text = "";
        protonPrefixField.text = "";
        winePrefixField.text = "";
        launchOptionsField.text = "";
        enableLoggingCheck.checked = false;
        iconField.text = "";
        gridField.text = "";
        heroField.text = "";
        logoField.text = "";
        steamGridDbIdField.text = "";
        steamIdField.text = "";
        platformCombo.currentIndex = -1;
        artSection.expanded = false;
        pendingAutoDownload = false;
        dialog.pendingInstallerRun = false;
        autoDownloadingInDialog = false;
        autoDownloadStatus = "";
        uzdoomModsModel.clear();
        runtimePicker.reset();
        prefixBasePath = protonScanner.prefixBasePath();
        dialog.open();
    }

    function applyExePath(path) {
        exeField.text = path;
        if (iconField.text === "") {
            var extracted = iconExtractor.extractIcon(path);
            if (extracted !== "")
                iconField.text = extracted;
        }
        if (nameField.text === "") {
            var parts = path.split("/");
            var filename = parts[parts.length - 1];
            if (/^\.desktop$/i.test(filename))
                nameField.text = filename.replace(/\.desktop$/i, "");
            else
                nameField.text = filename.replace(/\.(exe|sh|py|pl|rb|run|bash|zsh|AppImage|appimage|wad|pk3)$/i, "");
        }
        if (runtimePicker.runtimeType === "native" || runtimePicker.runtimeType === "uzdoom") {
            protonPrefixField.text = "";
            winePrefixField.text = "";
            return;
        }
        if (protonPrefixField.text === "")
            protonPrefixField.text = resolvePrefix("proton");
        if (winePrefixField.text === "")
            winePrefixField.text = resolvePrefix("wine");
    }

    function cleanupInstaller() {
        dialog.pendingInstallerRun = false;
        dialog.installerPrefixPath = "";
        dialog.installerRunning = false;
    }

    function runInstallerInPrefix() {
        if (!["proton", "wine"].includes(runtimePicker.resolvedRuntimeType))
            return;
        dialog.pendingInstallerRun = true;
        dialog.installerRunning = true;

        let resolvedPrefix = resolvePrefix();
        dialog.installerPrefixPath = resolvedPrefix;

        dialog.installerPid = launcher.runInPrefix({
            name: nameField.text,
            runtimeType: runtimePicker.resolvedRuntimeType,
            protonPath: runtimePicker.resolvedProtonPath,
            protonPrefix: resolvedPrefix,
            wineBinary: runtimePicker.resolvedWineBinary,
            winePrefix: resolvedPrefix,
            launchOptions: "",
            enableLogging: false
        }, dialog.installerExePath);

        if (dialog.installerPid <= 0) {
            dialog.validationError = i18n("Failed to start installer.");
            dialog.cleanupInstaller();
            return;
        }
    }

    function openForNewWithExe(exePath) {
        openForNew();
        if (!/\.exe$/i.test(exePath))
            runtimePicker.setRuntimeType("native");
        applyExePath(exePath);
    }

    function openForEdit(index) {
        editMode = true;
        editIndex = index;
        var app = appModel.getApp(index);
        nameField.text = app.name;
        exeField.text = app.exePath;
        protonPrefixField.text = app.protonPrefix;
        winePrefixField.text = app.winePrefix;
        launchOptionsField.text = app.launchOptions;
        enableLoggingCheck.checked = app.enableLogging;
        iconField.text = app.iconPath;
        gridField.text = app.gridPath || "";
        heroField.text = app.heroPath || "";
        logoField.text = app.logoPath || "";
        steamGridDbIdField.text = app.steamGridDbId > 0 ? app.steamGridDbId.toString() : "";
        steamIdField.text = app.steamAppId > 0 ? app.steamAppId.toString() : "";
        platformCombo.currentIndex = platformCombo.find(app.platformSlug);
        artSection.expanded = gridField.text !== "" || heroField.text !== "" || logoField.text !== "";
        prefixBasePath = protonScanner.prefixBasePath();
        pendingAutoDownload = false;
        autoDownloadingInDialog = false;
        autoDownloadStatus = "";
        installerExePath = "";
        uzdoomModsModel.clear();
        var mods = app.uzdoomMods || [];
        for (var mi = 0; mi < mods.length; mi++)
            uzdoomModsModel.append({
                "path": mods[mi]
            });
        runtimePicker.loadFromApp(app);
        dialog.open();
    }

    property string validationError: ""

    function collectUzdoomMods() {
        var mods = [];
        for (var i = 0; i < uzdoomModsModel.count; i++)
            mods.push(uzdoomModsModel.get(i).path);
        return mods;
    }

    function validate() {
        if (nameField.text.trim() === "") {
            validationError = i18n("Name is required.");
            return false;
        }
        if (exeField.text.trim() === "" && runtimePicker.runtimeType !== "steam") {
            validationError = i18n("Executable path is required.");
            return false;
        }
        var rtError = runtimePicker.validate();
        if (rtError !== "") {
            validationError = rtError;
            return false;
        }
        if (runtimePicker.runtimeType === "retroarch" && platformCombo.currentIndex < 0) {
            validationError = i18n("Please select a platform.");
            return false;
        }
        validationError = "";
        return true;
    }

    function resolvePrefix(runtimeType) {
        runtimeType = typeof runtimeType !== "undefined" && runtimeType !== null ? runtimeType : runtimePicker.runtimeType;
        var defaultPrefix = runtimeType === "wine" ? settingsManager.defaultWinePrefix : settingsManager.defaultGamePrefix;
        var basePath = runtimeType === "wine" ? protonScanner.winePrefixBasePath() : dialog.prefixBasePath;
        return defaultPrefix !== "" ? defaultPrefix : basePath + "/" + nameField.text.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase();
    }

    function doSave() {
        var rt = runtimePicker.runtimeType;
        var protonPath = runtimePicker.protonPath;
        var protonPrefix = protonPrefixField.text.trim() !== "" ? protonPrefixField.text : resolvePrefix("proton");
        var winePrefix = winePrefixField.text.trim() !== "" ? winePrefixField.text : resolvePrefix("wine");
        var sgdbId = parseInt(steamGridDbIdField.text);
        if (isNaN(sgdbId) || sgdbId < 0)
            sgdbId = 0;

        var app = {
            "name": nameField.text,
            "exePath": exeField.text,
            "runtimeType": rt,
            "steamAppId": rt === "steam" ? (steamIdField.text !== "" ? parseInt(steamIdField.text) : 0) : 0,
            "platformSlug": rt === "retroarch" ? (platformCombo.currentValue || "") : "",
            "uzdoomPath": runtimePicker.uzdoomPath,
            "uzdoomMods": dialog.collectUzdoomMods(),
            "protonPath": protonPath,
            "protonPrefix": protonPrefix,
            "wineBinary": runtimePicker.wineBinary,
            "winePrefix": winePrefix,
            "iconPath": iconField.text,
            "gridPath": gridField.text || "",
            "heroPath": heroField.text || "",
            "launchOptions": launchOptionsField.text || "",
            "enableLogging": enableLoggingCheck.checked,
            "logoPath": logoField.text || "",
            "steamGridDbId": sgdbId
        };

        if (editMode) {
            appModel.editApp(editIndex, app);
        } else {
            appModel.addApp(app);
        }

        if (!editMode && settingsManager.autoDownloadArt) {
            var saved = appModel.getAppByExePath(exeField.text);
            if (saved.id) {
                autoDownloadTargetId = saved.id;
                steamGridDb.autoDownloadAll(nameField.text, protonScanner.localAssetsPath(), settingsManager.steamGridDbApiKey);
            }
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            text: dialog.validationError
            visible: dialog.validationError !== ""
        }

        RuntimePicker {
            id: runtimePicker
            Layout.fillWidth: true
            twinFormLayouts: topForm
            showDefaultOption: true
        }

        Kirigami.FormLayout {
            id: topForm
            twinFormLayouts: runtimePicker.formLayout
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Game")
            }

            RowLayout {
                Layout.fillWidth: true
                visible: runtimePicker.runtimeType !== "steam"
                Kirigami.FormData.label: runtimePicker.runtimeType === "retroarch" ? i18n("ROM File:") : runtimePicker.runtimeType === "uzdoom" ? i18n("WAD File:") : runtimePicker.runtimeType === "native" ? i18n("Executable / AppImage:") : i18n("Executable (.exe):")
                QQC2.TextField {
                    id: exeField
                    Layout.fillWidth: true
                    placeholderText: runtimePicker.runtimeType === "retroarch" ? "/path/to/rom.sfc" : runtimePicker.runtimeType === "uzdoom" ? "/path/to/game.wad" : runtimePicker.runtimeType === "native" ? "/path/to/app.AppImage" : "/path/to/game.exe"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: runtimePicker.runtimeType === "retroarch" ? romFileDialog.open() : exeFileDialog.open()
                }
                QQC2.ToolButton {
                    visible: runtimePicker.resolvedRuntimeType === "wine" || runtimePicker.resolvedRuntimeType === "proton"
                    enabled: nameField.text.trim() !== "" && !dialog.installerRunning && runtimePicker.resolvedRuntimeType !== "" && runtimePicker.resolvedProtonPath !== ""
                    icon.name: dialog.installerRunning ? "content-loading-symbolic" : "system-run-symbolic"
                    QQC2.ToolTip.text: {
                        if (nameField.text.trim() === "")
                            return i18n("Please enter the game name before running an installer");
                        if (dialog.installerRunning)
                            return i18n("Installing...");
                        if (runtimePicker.resolvedRuntimeType !== "proton")
                            return i18n("Select Proton runtime first");
                        if (runtimePicker.resolvedProtonPath === "")
                            return i18n("Select a Proton version first");
                        return i18n("Run installer in prefix");
                    }
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: installerFileDialog.open()
                }
            }

            QQC2.TextField {
                id: nameField
                Layout.topMargin: 10
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Name:")
                placeholderText: i18n("My Game")
            }

            RowLayout {
                Layout.fillWidth: true
                visible: runtimePicker.runtimeType === "steam"
                Kirigami.FormData.label: i18n("Steam App ID:")
                QQC2.TextField {
                    id: steamIdField
                    Layout.fillWidth: true
                    placeholderText: "730"
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator {
                        bottom: 1
                    }
                }
            }

            QQC2.ComboBox {
                id: platformCombo
                visible: runtimePicker.runtimeType === "retroarch"
                Kirigami.FormData.label: i18n("Platform:")
                model: launcher.platformSlugs()
                textRole: "modelData"
                valueRole: "modelData"
                Layout.fillWidth: true
            }

            ColumnLayout {
                visible: runtimePicker.runtimeType === "uzdoom"
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.FormData.label: i18n("Mods:")
                Repeater {
                    model: uzdoomModsModel
                    delegate: RowLayout {
                        required property string path
                        required property int index
                        Layout.fillWidth: true
                        QQC2.TextField {
                            Layout.fillWidth: true
                            text: path
                            readOnly: true
                        }
                        QQC2.ToolButton {
                            icon.name: "edit-delete-symbolic"
                            QQC2.ToolTip.text: i18n("Remove mod")
                            QQC2.ToolTip.visible: hovered
                            onClicked: uzdoomModsModel.remove(index)
                        }
                    }
                }
                QQC2.Button {
                    text: i18n("Add mod...")
                    icon.name: "list-add-symbolic"
                    flat: true
                    onClicked: uzdoomModFileDialog.open()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Artwork")
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("SteamGridDB ID:")
                QQC2.TextField {
                    id: steamGridDbIdField
                    Layout.fillWidth: true
                    placeholderText: i18n("optional")
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator {
                        bottom: 1
                    }
                }
                QQC2.ToolButton {
                    icon.name: "system-search-symbolic"
                    enabled: nameField.text !== "" && settingsManager.steamGridDbApiKey !== "" && !steamGridDb.busy && !steamGridDb.autoDownloading
                    QQC2.ToolTip.text: settingsManager.steamGridDbApiKey === "" ? i18n("Set SteamGridDB API key in Settings") : i18n("Search SteamGridDB to set the ID")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: steamGridDbPicker.openPickerForId(nameField.text, settingsManager.steamGridDbApiKey)
                }
                QQC2.ToolButton {
                    icon.name: "folder-download-symbolic"
                    enabled: nameField.text !== "" && settingsManager.steamGridDbApiKey !== "" && !steamGridDb.autoDownloading && !steamGridDb.busy
                    QQC2.ToolTip.text: settingsManager.steamGridDbApiKey === "" ? i18n("Set SteamGridDB API key in Settings") : i18n("Auto-download all art from SteamGridDB")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: {
                        var storedId = parseInt(steamGridDbIdField.text);
                        if (!isNaN(storedId) && storedId > 0) {
                            dialog.autoDownloadingInDialog = true;
                            dialog.autoDownloadStatus = "";
                            steamGridDb.autoDownloadAllById(storedId, nameField.text, protonScanner.localAssetsPath(), settingsManager.steamGridDbApiKey);
                        } else {
                            dialog.pendingAutoDownload = true;
                            steamGridDbPicker.openPickerForId(nameField.text, settingsManager.steamGridDbApiKey);
                        }
                    }
                }
            }

            QQC2.Label {
                visible: dialog.autoDownloadStatus !== "" || steamGridDb.autoDownloading && dialog.autoDownloadingInDialog
                text: steamGridDb.autoDownloading && dialog.autoDownloadingInDialog ? steamGridDb.statusText : dialog.autoDownloadStatus
                opacity: 0.75
                font.italic: true
                Kirigami.FormData.label: ""
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Icon (optional):")
                QQC2.TextField {
                    id: iconField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/icon.png"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: iconFileDialog.open()
                }
                QQC2.ToolButton {
                    icon.name: "folder-download-symbolic"
                    enabled: nameField.text !== "" && settingsManager.steamGridDbApiKey !== "" && !steamGridDb.busy
                    QQC2.ToolTip.text: settingsManager.steamGridDbApiKey === "" ? i18n("Set SteamGridDB API key in Settings") : i18n("Download icon from SteamGridDB")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: {
                        var storedId = parseInt(steamGridDbIdField.text);
                        if (!isNaN(storedId) && storedId > 0)
                            steamGridDbPicker.openPickerWithId(storedId, nameField.text, "icon", settingsManager.steamGridDbApiKey, "icon");
                        else
                            steamGridDbPicker.openPicker(nameField.text, "icon", settingsManager.steamGridDbApiKey, "icon");
                    }
                }
            }

            QQC2.Button {
                Kirigami.FormData.label: ""
                text: artSection.expanded ? i18n("Hide Grid / Hero / Logo Art") : i18n("Show Grid / Hero / Logo Art")
                icon.name: artSection.expanded ? "go-up-symbolic" : "go-down-symbolic"
                flat: true
                onClicked: artSection.expanded = !artSection.expanded
            }

            QQC2.Button {
                Kirigami.FormData.label: ""
                visible: dialog.editMode
                text: i18n("Refresh artwork")
                icon.name: "view-refresh-symbolic"
                flat: true
                enabled: !steamGridDb.busy && nameField.text !== ""
                onClicked: {
                    if (settingsManager.steamGridDbApiKey !== "") {
                        steamGridDb.autoDownloadAll(nameField.text, protonScanner.localAssetsPath(), settingsManager.steamGridDbApiKey);
                    } else {
                        steamGridDb.autoDownloadFromBottles(nameField.text, protonScanner.localAssetsPath());
                        if (exeField.text !== "") {
                            var extracted = iconExtractor.extractIcon(exeField.text);
                            if (extracted !== "")
                                iconField.text = extracted;
                        }
                    }
                }
            }

            ColumnLayout {
                id: artSection
                property bool expanded: false
                visible: expanded
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    Layout.fillWidth: true
                    Kirigami.FormData.label: i18n("Grid (optional):")
                    QQC2.TextField {
                        id: gridField
                        Layout.fillWidth: true
                        placeholderText: "/path/to/grid.png"
                    }
                    QQC2.ToolButton {
                        icon.name: "document-open-symbolic"
                        onClicked: gridFileDialog.open()
                    }
                    QQC2.ToolButton {
                        icon.name: "folder-download-symbolic"
                        enabled: nameField.text !== "" && settingsManager.steamGridDbApiKey !== "" && !steamGridDb.busy
                        QQC2.ToolTip.text: settingsManager.steamGridDbApiKey === "" ? i18n("Set SteamGridDB API key in Settings") : i18n("Download grid from SteamGridDB")
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        onClicked: {
                            var storedId = parseInt(steamGridDbIdField.text);
                            if (!isNaN(storedId) && storedId > 0)
                                steamGridDbPicker.openPickerWithId(storedId, nameField.text, "grid", settingsManager.steamGridDbApiKey, "grid");
                            else
                                steamGridDbPicker.openPicker(nameField.text, "grid", settingsManager.steamGridDbApiKey, "grid");
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Kirigami.FormData.label: i18n("Hero (optional):")
                    QQC2.TextField {
                        id: heroField
                        Layout.fillWidth: true
                        placeholderText: "/path/to/hero.png"
                    }
                    QQC2.ToolButton {
                        icon.name: "document-open-symbolic"
                        onClicked: heroFileDialog.open()
                    }
                    QQC2.ToolButton {
                        icon.name: "folder-download-symbolic"
                        enabled: nameField.text !== "" && settingsManager.steamGridDbApiKey !== "" && !steamGridDb.busy
                        QQC2.ToolTip.text: settingsManager.steamGridDbApiKey === "" ? i18n("Set SteamGridDB API key in Settings") : i18n("Download hero from SteamGridDB")
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        onClicked: {
                            var storedId = parseInt(steamGridDbIdField.text);
                            if (!isNaN(storedId) && storedId > 0)
                                steamGridDbPicker.openPickerWithId(storedId, nameField.text, "hero", settingsManager.steamGridDbApiKey, "hero");
                            else
                                steamGridDbPicker.openPicker(nameField.text, "hero", settingsManager.steamGridDbApiKey, "hero");
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Kirigami.FormData.label: i18n("Logo (optional):")
                    QQC2.TextField {
                        id: logoField
                        Layout.fillWidth: true
                        placeholderText: "/path/to/logo.png"
                    }
                    QQC2.ToolButton {
                        icon.name: "document-open-symbolic"
                        onClicked: logoFileDialog.open()
                    }
                    QQC2.ToolButton {
                        icon.name: "folder-download-symbolic"
                        enabled: nameField.text !== "" && settingsManager.steamGridDbApiKey !== "" && !steamGridDb.busy
                        QQC2.ToolTip.text: settingsManager.steamGridDbApiKey === "" ? i18n("Set SteamGridDB API key in Settings") : i18n("Download logo from SteamGridDB")
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        onClicked: {
                            var storedId = parseInt(steamGridDbIdField.text);
                            if (!isNaN(storedId) && storedId > 0)
                                steamGridDbPicker.openPickerWithId(storedId, nameField.text, "logo", settingsManager.steamGridDbApiKey, "logo");
                            else
                                steamGridDbPicker.openPicker(nameField.text, "logo", settingsManager.steamGridDbApiKey, "logo");
                        }
                    }
                }
            }
        }

        Kirigami.FormLayout {
            twinFormLayouts: runtimePicker.formLayout
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Runtime Options")
                visible: runtimePicker.runtimeType !== "steam"
            }

            RowLayout {
                Layout.fillWidth: true
                visible: runtimePicker.resolvedRuntimeType === "proton"
                Kirigami.FormData.label: i18n("Proton Prefix (optional):")
                QQC2.TextField {
                    id: protonPrefixField
                    Layout.fillWidth: true
                    placeholderText: settingsManager.defaultGamePrefix !== "" ? settingsManager.defaultGamePrefix : dialog.prefixBasePath + "/mygame"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: prefixFolderDialog.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: runtimePicker.resolvedRuntimeType === "wine"
                Kirigami.FormData.label: i18n("Wine Prefix (WINEPREFIX):")
                QQC2.TextField {
                    id: winePrefixField
                    Layout.fillWidth: true
                    placeholderText: settingsManager.defaultWinePrefix !== "" ? settingsManager.defaultWinePrefix : protonScanner.winePrefixBasePath() + "/mygame"
                }
                QQC2.ToolButton {
                    icon.name: "document-open-symbolic"
                    onClicked: winePrefixFolderDialog.open()
                }
            }

            QQC2.TextField {
                id: launchOptionsField
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Launch Options (optional):")
                placeholderText: i18n("e.g. mangohud %command%")
                visible: runtimePicker.runtimeType !== "steam"
            }

            QQC2.CheckBox {
                id: enableLoggingCheck
                text: i18n("Write output to log file")
                visible: runtimePicker.runtimeType !== "steam"
            }

            Repeater {
                model: settingsManager.globalEnvVars
                delegate: QQC2.Label {
                    Kirigami.FormData.label: index === 0 ? i18n("Global Env Vars:") : ""
                    text: modelData
                    font.family: "monospace"
                    opacity: 0.7
                }
            }
        }
    }

    FileDialog {
        id: exeFileDialog
        title: i18n("Select Executable")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: runtimePicker.runtimeType === "native" ? [i18n("Binaries, scripts & AppImages (*.sh *.py *.pl *.rb *.run *.bash *.zsh *.AppImage *.appimage *.desktop)"), i18n("All files (*)")] : runtimePicker.runtimeType === "uzdoom" ? [i18n("WAD files (*.wad)"), i18n("All files (*)")] : [i18n("Executables (*.exe)"), i18n("All files (*)")]
        onAccepted: {
            var path = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            dialog.applyExePath(path);
        }
    }

    FileDialog {
        id: uzdoomModFileDialog
        title: i18n("Select mod (WAD or PK3)")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("WAD & PK3 files (*.wad *.pk3)"), i18n("All files (*)")]
        onAccepted: {
            var path = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            for (var i = 0; i < uzdoomModsModel.count; i++) {
                if (uzdoomModsModel.get(i).path === path)
                    return;
            }
            uzdoomModsModel.append({
                "path": path
            });
        }
    }

    ListModel {
        id: uzdoomModsModel
    }

    FileDialog {
        id: installerFileDialog
        title: i18n("Select installer (exe)")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Executables (*.exe)"), i18n("All files (*)")]

        onAccepted: {
            dialog.installerExePath = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            dialog.cleanupInstaller();
            dialog.runInstallerInPrefix();
        }
        onRejected: {
            dialog.cleanupInstaller();
        }
    }

    FileDialog {
        id: romFileDialog
        title: i18n("Select ROM")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: {
            var path = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            exeField.text = path;
            if (nameField.text === "") {
                var parts = path.split("/");
                nameField.text = parts[parts.length - 1].replace(/\.[^.]+$/, "");
            }
        }
    }

    FileDialog {
        id: iconFileDialog
        title: i18n("Select Icon")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Images (*.png *.svg *.ico *.jpg)"), i18n("All files (*)")]
        onAccepted: iconField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }

    FolderDialog {
        id: prefixFolderDialog
        title: i18n("Select Proton Prefix Folder")
        currentFolder: "file://" + dialog.prefixBasePath
        onAccepted: protonPrefixField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: winePrefixFolderDialog
        title: i18n("Select Wine Prefix Folder")
        currentFolder: "file://" + protonScanner.winePrefixBasePath()
        onAccepted: winePrefixField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FileDialog {
        id: gridFileDialog
        title: i18n("Select Grid Image")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Images (*.png *.jpg *.jpeg *.webp)"), i18n("All files (*)")]
        onAccepted: gridField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }

    FileDialog {
        id: heroFileDialog
        title: i18n("Select Hero Image")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Images (*.png *.jpg *.jpeg *.webp)"), i18n("All files (*)")]
        onAccepted: heroField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }

    FileDialog {
        id: logoFileDialog
        title: i18n("Select Logo Image")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Images (*.png *.jpg *.jpeg *.webp)"), i18n("All files (*)")]
        onAccepted: logoField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }

    SteamGridDBPickerDialog {
        id: steamGridDbPicker
        onArtSelected: function (path) {
            if (steamGridDbPicker.targetField === "icon")
                iconField.text = path;
            else if (steamGridDbPicker.targetField === "grid")
                gridField.text = path;
            else if (steamGridDbPicker.targetField === "hero")
                heroField.text = path;
            else if (steamGridDbPicker.targetField === "logo")
                logoField.text = path;
        }
        onGameIdFound: function (id) {
            steamGridDbIdField.text = id.toString();
            if (dialog.pendingAutoDownload) {
                dialog.pendingAutoDownload = false;
                dialog.autoDownloadingInDialog = true;
                dialog.autoDownloadStatus = "";
                steamGridDb.autoDownloadAllById(id, nameField.text, protonScanner.localAssetsPath(), settingsManager.steamGridDbApiKey);
            }
        }
    }

    Connections {
        target: steamGridDb
        function onAutoDownloadFinished(gameId, iconPath, gridPath, heroPath, logoPath) {
            if (dialog.autoDownloadTargetId !== "") {
                appModel.updateAppArt(dialog.autoDownloadTargetId, iconPath, gridPath, heroPath, logoPath, gameId);
                dialog.autoDownloadTargetId = "";
            } else if (dialog.autoDownloadingInDialog) {
                dialog.autoDownloadingInDialog = false;
                dialog.autoDownloadStatus = i18n("Art downloaded!");
                if (iconPath !== "")
                    iconField.text = iconPath;
                if (gridPath !== "") {
                    gridField.text = gridPath;
                    artSection.expanded = true;
                }
                if (heroPath !== "") {
                    heroField.text = heroPath;
                    artSection.expanded = true;
                }
                if (logoPath !== "") {
                    logoField.text = logoPath;
                    artSection.expanded = true;
                }
                if (gameId > 0)
                    steamGridDbIdField.text = gameId.toString();
            }
        }
        function onAutoDownloadProgress(step) {
            if (dialog.autoDownloadingInDialog)
                dialog.autoDownloadStatus = step;
        }
    }

    Connections {
        target: launcher
        function onRunningExePathsChanged() {
            if (!dialog.installerRunning || dialog.installerExePath === "")
                return;

            let installerFinished = false;

            if (dialog.installerPid && dialog.installerPid > 0) {
                let currentPid = launcher.runningPidForExe(dialog.installerExePath);
                if (currentPid !== dialog.installerPid)
                    installerFinished = true;
            } else {
                let stillRunning = launcher.runningExePaths.indexOf(dialog.installerExePath) >= 0;
                if (!stillRunning)
                    installerFinished = true;
            }

            if (!installerFinished)
                return;

            dialog.installerRunning = false;
            dialog.installerExePath = "";
            dialog.installerPid = 0;

            // Fill the prefix field with the prefix used for installation
            if (dialog.installerPrefixPath !== "") {
                if (runtimePicker.runtimeType === "proton")
                    protonPrefixField.text = dialog.installerPrefixPath;
                else if (runtimePicker.runtimeType === "wine")
                    winePrefixField.text = dialog.installerPrefixPath;

                // Open exe picker starting in the prefix folder
                exeFileDialog.currentFolder = "file://" + dialog.installerPrefixPath;
                exeFileDialog.open();
            }

            dialog.cleanupInstaller();
        }
    }
}
