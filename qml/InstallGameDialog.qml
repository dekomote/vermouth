import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: i18n("Install a Game")
    preferredWidth: Kirigami.Units.gridUnit * 34
    padding: Kirigami.Units.largeSpacing
    bottomPadding: 30
    standardButtons: Kirigami.Dialog.NoButton

    property string validationError: ""
    property bool downloading: false
    property bool installing: false
    property string statusText: ""
    property int installPid: 0
    property string installPrefix: ""
    property string installExe: ""
    property string finalExe: ""
    property string artTargetId: ""
    property var artQueue: []

    signal installed(string name)

    property var launcherList: [
        {
            "key": "",
            "name": i18n("None - choose an installer exe"),
            "url": "",
            "file": "",
            "exe": ""
        },
        {
            "key": "amazon",
            "name": "Amazon Games",
            "url": "https://download.amazongames.com/AmazonGamesSetup.exe",
            "file": "AmazonGamesSetup.exe",
            "exe": "drive_c/users/steamuser/AppData/Local/Amazon Games/App/Amazon Games.exe"
        },
        {
            "key": "battle",
            "name": "Battle.net",
            "url": "https://downloader.battle.net/download/getInstaller?os=win&installer=Battle.net-Setup.exe",
            "file": "Battle.net-Setup.exe",
            "exe": "drive_c/Program Files (x86)/Battle.net/Battle.net.exe"
        },
        {
            "key": "ea",
            "name": "EA App",
            "url": "https://origin-a.akamaihd.net/EA-Desktop-Client-Download/installer-releases/EAappInstaller.exe",
            "file": "EAappInstaller.exe",
            "exe": "drive_c/Program Files/Electronic Arts/EA Desktop/EA Desktop/EALauncher.exe"
        },
        {
            "key": "epic",
            "name": "Epic Games Launcher",
            "url": "https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/installer/download/EpicGamesLauncherInstaller.msi",
            "file": "EpicGamesLauncherInstaller.msi",
            "exe": "drive_c/Program Files/Epic Games/Launcher/Portal/Binaries/Win64/EpicGamesLauncher.exe"
        },
        {
            "key": "rockstar",
            "name": "Rockstar Games",
            "url": "https://gamedownloads.rockstargames.com/public/installer/Rockstar-Games-Launcher.exe",
            "file": "Rockstar-Games-Launcher.exe",
            "exe": "drive_c/Program Files/Rockstar Games/Launcher/Launcher.exe"
        },
        {
            "key": "ubisoft",
            "name": "Ubisoft Connect",
            "url": "https://static3.cdn.ubi.com/orbit/launcher_installer/UbisoftConnectInstaller.exe",
            "file": "UbisoftConnectInstaller.exe",
            "exe": "drive_c/Program Files (x86)/Ubisoft/Ubisoft Game Launcher/UbisoftConnect.exe"
        },
        {
            "key": "wargaming",
            "name": "Wargaming Game Center",
            "url": "https://redirect.wargaming.net/WGC/Wargaming_Game_Center_Install_NA.exe",
            "file": "Wargaming-Game-Center-Install-NA.exe",
            "exe": "drive_c/ProgramData/Wargaming.net/GameCenter/wgc.exe"
        }
    ]

    function openDialog() {
        reset();
        dialog.open();
    }

    function reset() {
        validationError = "";
        downloading = false;
        installing = false;
        statusText = "";
        installPid = 0;
        installPrefix = "";
        installExe = "";
        finalExe = "";
        nameField.text = "";
        launcherCombo.currentIndex = 0;
    }

    function selectedLauncher() {
        return dialog.launcherList[launcherCombo.currentIndex];
    }

    function effectiveRuntime() {
        return (settingsManager.defaultRuntimeType || "").trim();
    }

    function generatedPrefix() {
        var rt = effectiveRuntime();
        var basePath = rt === "wine" ? protonScanner.winePrefixBasePath() : protonScanner.prefixBasePath();
        return basePath + "/" + nameField.text.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase();
    }

    function startInstall() {
        if (nameField.text.trim() === "") {
            validationError = i18n("Name is required.");
            return;
        }
        var rt = effectiveRuntime();
        if (rt === "") {
            validationError = i18n("Set your default runtime (Proton or Wine) in Settings first.");
            return;
        }
        validationError = "";
        installPrefix = generatedPrefix();

        var launcher = selectedLauncher();
        if (launcher.key !== "") {
            dialog.finalExe = dialog.installPrefix + "/" + launcher.exe;
            dialog.downloading = true;
            dialog.statusText = i18n("Downloading %1…", launcher.name);
            launcherDownloader.download(launcher.url, launcher.file);
        } else {
            dialog.finalExe = "";
            exeFileDialog.open();
        }
    }

    function runInstallerInPrefix(exePath, launchOptions, finalExePath) {
        var rt = effectiveRuntime();
        var app = {
            "name": nameField.text.trim(),
            "runtimeType": rt,
            "protonPath": settingsManager.defaultProtonPath,
            "wineBinary": settingsManager.defaultWineBinary,
            "protonPrefix": dialog.installPrefix,
            "winePrefix": dialog.installPrefix,
            "launchOptions": launchOptions || "",
            "enableLogging": false
        };
        dialog.installExe = exePath;
        if (finalExePath !== "")
            dialog.finalExe = finalExePath;
        dialog.statusText = i18n("Installing in prefix…");
        dialog.installing = true;
        dialog.installPid = launcher.runInPrefix(app, exePath);
        if (dialog.installPid <= 0) {
            dialog.installing = false;
            dialog.installPid = 0;
            dialog.statusText = i18n("Failed to start the installer.");
        }
    }

    function startArtDownload() {
        if (steamGridDb.autoDownloading || dialog.artQueue.length === 0)
            return;
        var item = dialog.artQueue[0];
        dialog.artTargetId = item.id;
        steamGridDb.autoDownloadAll(item.name, protonScanner.localAssetsPath(), settingsManager.steamGridDbApiKey);
    }

    function finishInstall() {
        dialog.installing = false;
        dialog.installPid = 0;

        var exe = dialog.finalExe;
        if (exe !== "") {
            var app = {
                "name": nameField.text.trim(),
                "exePath": exe,
                "runtimeType": effectiveRuntime(),
                "protonPath": settingsManager.defaultProtonPath,
                "protonPrefix": dialog.installPrefix,
                "wineBinary": settingsManager.defaultWineBinary,
                "winePrefix": dialog.installPrefix,
                "iconPath": "",
                "launchOptions": ""
            };
            var extracted = iconExtractor.extractIcon(exe);
            if (extracted !== "")
                app.iconPath = extracted;
            appModel.addApp(app);
            var installedName = nameField.text.trim();

            if (settingsManager.autoDownloadArt) {
                var saved = appModel.getAppByExePath(exe);
                if (saved.id) {
                    dialog.artQueue.push({
                        "id": saved.id,
                        "name": installedName
                    });
                    dialog.startArtDownload();
                }
            }

            reset();
            dialog.close();
            dialog.installed(installedName);
            return;
        }

        dialog.statusText = i18n("Installer finished. Pick the installed executable.");
    }

    customFooterActions: [
        Kirigami.Action {
            text: dialog.installing || dialog.downloading ? i18n("Installing…") : i18n("Install")
            icon.name: "media-playback-start-symbolic"
            enabled: !dialog.downloading && !dialog.installing && nameField.text.trim() !== ""
            onTriggered: dialog.startInstall()
        },
        Kirigami.Action {
            text: i18n("Cancel")
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            text: dialog.validationError
            visible: dialog.validationError !== ""
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QQC2.TextField {
                id: nameField
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Game name:")
                placeholderText: i18n("My Game")
            }

            QQC2.ComboBox {
                id: launcherCombo
                Layout.fillWidth: true
                Kirigami.FormData.label: i18n("Launcher:")
                model: dialog.launcherList
                textRole: "name"
            }

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Information
                visible: selectedLauncher().key !== ""
                text: i18n("Use the default install paths in the launcher setup, and close the launcher when it first opens after installing — otherwise the game won't be added correctly.")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                Layout.fillWidth: true
                visible: dialog.installPrefix !== ""
                text: i18n("Prefix: %1", dialog.installPrefix)
                opacity: 0.7
                elide: Text.ElideMiddle
            }

            DownloaderProgress {
                Layout.fillWidth: true
                Kirigami.FormData.label: ""
                visible: dialog.downloading
                progress: launcherDownloader.progress
                statusText: launcherDownloader.statusText
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                Layout.fillWidth: true
                visible: dialog.statusText !== "" && !dialog.downloading
                text: dialog.statusText
                opacity: 0.75
                font.italic: true
                wrapMode: Text.WordWrap
            }
        }
    }

    FileDialog {
        id: exeFileDialog
        title: i18n("Select Installer Executable")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Executables (*.exe *.msi)"), i18n("All files (*)")]
        onAccepted: {
            var path = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            dialog.finalExe = path;
            dialog.runInstallerInPrefix(path, "", path);
        }
    }

    Connections {
        target: launcherDownloader
        function onFinished(filePath) {
            if (!dialog.downloading)
                return;
            dialog.downloading = false;
            dialog.statusText = "";
            dialog.runInstallerInPrefix(filePath, "", dialog.finalExe);
        }
        function onError(message) {
            if (!dialog.downloading)
                return;
            dialog.downloading = false;
            dialog.statusText = i18n("Could not download the launcher: %1", message);
        }
    }

    Connections {
        target: steamGridDb
        function onAutoDownloadFinished(gameId, iconPath, gridPath, heroPath, logoPath) {
            if (dialog.artQueue.length === 0)
                return;
            appModel.updateAppArt(dialog.artQueue[0].id, iconPath, gridPath, heroPath, logoPath, gameId);
            dialog.artQueue.shift();
            dialog.artTargetId = "";
            dialog.startArtDownload();
        }
    }

    Connections {
        target: launcher
        function onRunningExePathsChanged() {
            if (!dialog.installing || dialog.installExe === "")
                return;

            let installerFinished = false;
            if (dialog.installPid && dialog.installPid > 0) {
                let currentPid = launcher.runningPidForExe(dialog.installExe);
                if (currentPid !== dialog.installPid)
                    installerFinished = true;
            } else {
                if (launcher.runningExePaths.indexOf(dialog.installExe) < 0)
                    installerFinished = true;
            }

            if (installerFinished)
                dialog.finishInstall();
        }
    }
}
