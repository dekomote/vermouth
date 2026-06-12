import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: settingsPage
    title: i18n("Settings")

    function save() {
        settingsManager.setUmuPath(umuPathField.text);
        settingsManager.setDefaultPrefixDir(prefixDirField.text);
        settingsManager.setDefaultGamePrefix(gamePrefixField.text);
        settingsManager.setDefaultWinePrefix(winePrefixField.text);
        settingsManager.setGamepadFullscreenButton(gamepadFullscreenCombo.currentValue);
        settingsManager.setSteamGridDbApiKey(steamGridDbKeyField.text);
        settingsManager.setRommServerUrl(rommUrlField.text.trim().replace(/\/+$/, ""));
        settingsManager.setRommApiKey(rommApiKeyField.text);
        settingsManager.setRetroarchPath(retroarchPathField.text);
        settingsManager.setRomCacheDir(romCacheDirField.text);
        settingsManager.setGogCacheDir(gogCacheDirField.text);
        settingsManager.setGogInstallDir(gogInstallDirField.text);
        defaultRuntimePicker.saveToSettings();
        var vars = [];
        for (var i = 0; i < envModel.count; i++) {
            var k = envModel.get(i).key.trim();
            var v = envModel.get(i).value.trim();
            if (k !== "")
                vars.push(k + "=" + v);
        }
        settingsManager.setGlobalEnvVars(vars);
    }

    function load() {
        umuPathField.text = settingsManager.umuPath;
        prefixDirField.text = settingsManager.defaultPrefixDir;
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
        gogCacheDirField.text = settingsManager.gogCacheDir;
        gogInstallDirField.text = settingsManager.gogInstallDir;
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
                    icon.name: "document-open"
                    onClicked: umuFilePicker.open()
                }
                QQC2.ToolButton {
                    icon.name: "download"
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
                    icon.name: "document-open"
                    onClicked: prefixDirFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("This is the folder where Vermouth stores all the created prefixes by default.")
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
                    icon.name: "document-open"
                    onClicked: gamePrefixFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Set this if you want all Proton games to share a single prefix (e.g. one Proton environment for everything). Leave empty to auto-generate a separate prefix per game. You can still use separate prefix per game, but you have to set it explicitly.")
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
                    icon.name: "document-open"
                    onClicked: winePrefixFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Set this if you want all Wine games to share a single prefix. Leave empty to auto-generate a separate Wine prefix per game under the 'wines/' subfolder.")
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Vermouth Proton Folder")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("Download GE Proton to run most games and apps — no Steam or manual setup needed.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.pointSize: Kirigami.Theme.defaultFont.pointSize - 2
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("GE Proton:")
                QQC2.Button {
                    icon.name: "folder-open"
                    text: i18n("Open Vermouth Proton folder")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: protonScanner.localProtonPath()
                    onClicked: Qt.openUrlExternally("file://" + protonScanner.localProtonPath())
                }
                QQC2.Button {
                    text: i18n("Download Latest GE Proton")
                    icon.name: "download"
                    enabled: !protonDownloader.busy
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: protonDownloader.statusText ? protonDownloader.statusText : i18n("Download latest GE Proton")
                    onClicked: protonDownloader.downloadLatest()
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
                    icon.name: "document-open"
                    onClicked: romCacheFolderDialog.open()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("GOG")
            }

            QQC2.Label {
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
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Account:")
                QQC2.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: gogClient.authenticated ? (gogClient.username !== "" ? i18n("Logged in as %1", gogClient.username) : i18n("Logged in")) : i18n("Not logged in")
                }
                QQC2.Button {
                    text: i18n("Log out")
                    icon.name: "system-log-out"
                    enabled: gogClient.authenticated
                    onClicked: gogClient.logout()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Download Folder:")
                QQC2.TextField {
                    id: gogCacheDirField
                    Layout.fillWidth: true
                    placeholderText: i18n("Default: AppData/gog")
                }
                QQC2.ToolButton {
                    icon.name: "document-open"
                    onClicked: gogCacheFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Where GOG installers are downloaded before installation.")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Linux Install Folder:")
                QQC2.TextField {
                    id: gogInstallDirField
                    Layout.fillWidth: true
                    placeholderText: i18n("Default: ~/GOG Games")
                }
                QQC2.ToolButton {
                    icon.name: "document-open"
                    onClicked: gogInstallFolderDialog.open()
                }
                Kirigami.ContextualHelpButton {
                    toolTipText: i18n("Where native Linux GOG games are installed. Windows games install into a Proton or Wine prefix.")
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
                    icon.name: "document-open"
                    onClicked: retroarchFilePicker.open()
                }
                QQC2.ToolButton {
                    icon.name: "view-refresh"
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
                    icon.name: "list-add"
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

                QQC2.Button {
                    text: i18n("Add Variable")
                    icon.name: "list-add"
                    onClicked: envModel.append({
                        "key": "",
                        "value": ""
                    })
                }
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
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Lights Out:")
                QQC2.Switch {
                    checked: settingsManager.lightsOut
                    onToggled: settingsManager.setLightsOut(checked)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Background Color:")
                opacity: settingsManager.lightsOut ? 1.0 : 0.5

                Rectangle {
                    width: Kirigami.Units.gridUnit * 4
                    height: Kirigami.Units.gridUnit * 1.5
                    color: settingsManager.lightsOutColor
                    radius: Kirigami.Units.cornerRadius
                    border.color: Kirigami.Theme.disabledTextColor
                    border.width: 1

                    MouseArea {
                        anchors.fill: parent
                        enabled: settingsManager.lightsOut
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            lightsOutColorDialog.selectedColor = settingsManager.lightsOutColor;
                            lightsOutColorDialog.open();
                        }
                    }
                }

                QQC2.Button {
                    text: i18n("Reset")
                    enabled: settingsManager.lightsOut
                    onClicked: settingsManager.setLightsOutColor("#2A2E32")
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
        }
    }

    FileDialog {
        id: umuFilePicker
        title: i18n("Select umu-run binary")
        currentFolder: umuPathField.text !== "" ? "file://" + umuPathField.text.substring(0, umuPathField.text.lastIndexOf("/")) : "file://" + protonScanner.homePath()
        onAccepted: umuPathField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
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
        title: i18n("Select GOG Linux Install Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: gogInstallDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    ColorDialog {
        id: lightsOutColorDialog
        title: i18n("Select Background Color")
        onAccepted: {
            var r = Math.round(selectedColor.r * 255);
            var g = Math.round(selectedColor.g * 255);
            var b = Math.round(selectedColor.b * 255);
            settingsManager.setLightsOutColor("#" + r.toString(16).padStart(2, "0") + g.toString(16).padStart(2, "0") + b.toString(16).padStart(2, "0"));
        }
    }
}
