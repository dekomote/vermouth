import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: editMode ? i18n("Edit App/Game") : i18n("Add App/Game")
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
    property string autoDownloadStatus: ""

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
        autoDownloadingInDialog = false;
        autoDownloadStatus = "";
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
            if (/\.desktop$/i.test(filename))
                nameField.text = filename.replace(/\.desktop$/i, "");
            else
                nameField.text = filename.replace(/\.(exe|sh|py|pl|rb|run|bash|zsh|AppImage|appimage)$/i, "");
        }
        if (runtimePicker.runtimeType === "native") {
            protonPrefixField.text = "";
            winePrefixField.text = "";
            return;
        }
        var defaultPrefix = settingsManager.defaultGamePrefix;
        var resolvedPrefix = defaultPrefix !== "" ? defaultPrefix : dialog.prefixBasePath + "/" + nameField.text.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase();
        if (protonPrefixField.text === "")
            protonPrefixField.text = resolvedPrefix;
        if (winePrefixField.text === "")
            winePrefixField.text = resolvedPrefix;
    }

    function openForNewLinux() {
        openForNew();
        runtimePicker.setRuntimeType("native");
    }

    function openForNewWindows() {
        openForNew();
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
        runtimePicker.loadFromApp(app);
        dialog.open();
    }

    property string validationError: ""

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

    function resolvePrefix() {
        var defaultPrefix = settingsManager.defaultGamePrefix;
        return defaultPrefix !== "" ? defaultPrefix : dialog.prefixBasePath + "/" + nameField.text.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase();
    }

    function doSave() {
        var rt = runtimePicker.runtimeType;
        var protonPath = runtimePicker.protonPath;
        var protonPrefix = protonPrefixField.text.trim() !== "" ? protonPrefixField.text : resolvePrefix();
        var winePrefix = winePrefixField.text.trim() !== "" ? winePrefixField.text : resolvePrefix();
        var sgdbId = parseInt(steamGridDbIdField.text);
        if (isNaN(sgdbId) || sgdbId < 0)
            sgdbId = 0;

        var app = {
            "name": nameField.text,
            "exePath": exeField.text,
            "runtimeType": rt,
            "steamAppId": rt === "steam" ? (steamIdField.text !== "" ? parseInt(steamIdField.text) : 0) : 0,
            "platformSlug": rt === "retroarch" ? (platformCombo.currentValue || "") : "",
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

        if (!editMode && settingsManager.autoDownloadArt && settingsManager.steamGridDbApiKey !== "") {
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

        Kirigami.FormLayout {
            id: topForm
            twinFormLayouts: runtimePicker.formLayout

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Game")
            }

            QQC2.TextField {
                id: nameField
                Layout.topMargin: 10
                Kirigami.FormData.label: i18n("Name:")
                placeholderText: i18n("My Game")
            }

            RowLayout {
                visible: runtimePicker.runtimeType !== "steam"
                Kirigami.FormData.label: runtimePicker.runtimeType === "retroarch" ? i18n("ROM File:") : runtimePicker.runtimeType === "native" ? i18n("Executable / AppImage:") : i18n("Executable (.exe):")
                QQC2.TextField {
                    id: exeField
                    Layout.fillWidth: true
                    placeholderText: runtimePicker.runtimeType === "retroarch" ? "/path/to/rom.sfc" : runtimePicker.runtimeType === "native" ? "/path/to/app.AppImage" : "/path/to/game.exe"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: runtimePicker.runtimeType === "retroarch" ? romFileDialog.open() : exeFileDialog.open()
                }
            }

            RowLayout {
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
            }

            RowLayout {
                Kirigami.FormData.label: i18n("SteamGridDB ID:")
                QQC2.TextField {
                    id: steamGridDbIdField
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 8
                    placeholderText: i18n("optional")
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator {
                        bottom: 1
                    }
                }
                QQC2.Button {
                    icon.name: "search"
                    enabled: nameField.text !== "" && settingsManager.steamGridDbApiKey !== "" && !steamGridDb.busy && !steamGridDb.autoDownloading
                    QQC2.ToolTip.text: settingsManager.steamGridDbApiKey === "" ? i18n("Set SteamGridDB API key in Settings") : i18n("Search SteamGridDB to set the ID")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: steamGridDbPicker.openPickerForId(nameField.text, settingsManager.steamGridDbApiKey)
                }
                QQC2.Button {
                    icon.name: "download"
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
                Kirigami.FormData.label: i18n("Icon (optional):")
                QQC2.TextField {
                    id: iconField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/icon.png"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: iconFileDialog.open()
                }
                QQC2.Button {
                    icon.name: "download"
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
                icon.name: artSection.expanded ? "arrow-up" : "arrow-down"
                flat: true
                onClicked: artSection.expanded = !artSection.expanded
            }

            ColumnLayout {
                id: artSection
                property bool expanded: false
                visible: expanded
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    Kirigami.FormData.label: i18n("Grid (optional):")
                    QQC2.TextField {
                        id: gridField
                        Layout.fillWidth: true
                        placeholderText: "/path/to/grid.png"
                    }
                    QQC2.Button {
                        icon.name: "document-open"
                        onClicked: gridFileDialog.open()
                    }
                    QQC2.Button {
                        icon.name: "download"
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
                    Kirigami.FormData.label: i18n("Hero (optional):")
                    QQC2.TextField {
                        id: heroField
                        Layout.fillWidth: true
                        placeholderText: "/path/to/hero.png"
                    }
                    QQC2.Button {
                        icon.name: "document-open"
                        onClicked: heroFileDialog.open()
                    }
                    QQC2.Button {
                        icon.name: "download"
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
                    Kirigami.FormData.label: i18n("Logo (optional):")
                    QQC2.TextField {
                        id: logoField
                        Layout.fillWidth: true
                        placeholderText: "/path/to/logo.png"
                    }
                    QQC2.Button {
                        icon.name: "document-open"
                        onClicked: logoFileDialog.open()
                    }
                    QQC2.Button {
                        icon.name: "download"
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

        RuntimePicker {
            id: runtimePicker
            Layout.fillWidth: true
            twinFormLayouts: topForm
        }

        Kirigami.FormLayout {
            twinFormLayouts: runtimePicker.formLayout

            RowLayout {
                visible: runtimePicker.runtimeType === "proton"
                Kirigami.FormData.label: i18n("Proton Prefix (optional):")
                QQC2.TextField {
                    id: protonPrefixField
                    Layout.fillWidth: true
                    placeholderText: settingsManager.defaultGamePrefix !== "" ? settingsManager.defaultGamePrefix : dialog.prefixBasePath + "/mygame"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: prefixFolderDialog.open()
                }
            }

            RowLayout {
                visible: runtimePicker.runtimeType === "wine"
                Kirigami.FormData.label: i18n("Wine Prefix (WINEPREFIX):")
                QQC2.TextField {
                    id: winePrefixField
                    Layout.fillWidth: true
                    placeholderText: settingsManager.defaultGamePrefix !== "" ? settingsManager.defaultGamePrefix : "~/.wine"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: winePrefixFolderDialog.open()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Options")
            }

            QQC2.TextField {
                id: launchOptionsField
                visible: runtimePicker.runtimeType !== "steam"
                Kirigami.FormData.label: i18n("Launch Options (optional):")
                placeholderText: i18n("e.g. mangohud %command%")
            }

            QQC2.CheckBox {
                id: enableLoggingCheck
                visible: runtimePicker.runtimeType !== "steam"
                text: i18n("Write output to log file")
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
        nameFilters: runtimePicker.runtimeType === "native" ? [i18n("Binaries, scripts & AppImages (*.sh *.py *.pl *.rb *.run *.bash *.zsh *.AppImage *.appimage *.desktop)"), i18n("All files (*)")] : [i18n("Executables (*.exe)"), i18n("All files (*)")]
        onAccepted: {
            var path = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            dialog.applyExePath(path);
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
        currentFolder: "file://" + dialog.prefixBasePath
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
}
