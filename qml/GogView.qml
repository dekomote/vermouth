import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GameGridView {
    id: gogGrid
    model: gogLibraryModel

    property bool hasMore: false
    property string searchText: ""

    // In-flight download/install state (only one at a time). pendingPhase is one
    // of "", "resolve", "download", "install" and drives the in-cell overlay.
    property string pendingGameId: ""
    property string pendingGameName: ""
    property string pendingPhase: ""
    property bool pendingIsWindows: true
    property string pendingPrefix: ""
    property string pendingRuntimeType: "proton"
    property string pendingRuntimePath: ""
    property string pendingInstallerPath: ""
    property bool pendingManual: false
    property string artAppId: ""

    function resetPending() {
        pendingGameId = "";
        pendingPhase = "";
        pendingInstallerPath = "";
        pendingManual = false;
    }

    function sanitize(name) {
        return name.replace(/[^a-zA-Z0-9_-]/g, "_");
    }

    function applySearch(text) {
        searchText = text;
        searchDebounce.stop();
        if (gogClient.authenticated)
            searchDebounce.restart();
    }

    function refresh() {
        if (gogClient.authenticated)
            gogLibraryModel.fetchLibrary("", 1);
    }

    function checkLoadMore() {
        if (hasMore && !gogLibraryModel.busy && (contentHeight <= height || contentY + height >= contentHeight - cellHeight * 2))
            gogLibraryModel.fetchNextPage(searchText);
    }

    function launchOrInstall(index) {
        if (index < 0)
            return;
        var g = gogLibraryModel.getGame(index);
        if (g.installed && g.exePath !== "") {
            var app = appModel.getAppByExePath(g.exePath);
            if (app.exePath !== undefined) {
                launcher.launchEntry(app);
                return;
            }
        }
        startInstall(g);
    }

    function startInstall(g, manual) {
        if (gogDownloader.busy || gogInstaller.busy || pendingGameId !== "") {
            showPassiveNotification(i18n("A download or install is already in progress"), 3000);
            return;
        }
        pendingGameId = g.gameId;
        pendingGameName = g.title;
        pendingManual = manual === true;
        pendingPhase = "resolve";
        gogClient.fetchDownloadInfo(g.gameId, !pendingManual && g.worksOnLinux === true);
    }

    function promptManualInstall() {
        manualInstallDialog.open();
    }

    function doManualInstall() {
        runWindowsInstaller(false);
    }

    function installWineBinary() {
        var bin = settingsManager.defaultWineBinary;
        if (bin !== "" && wineScanner.isInstalled(bin))
            return bin;
        var wv = wineScanner.findWineVersions();
        return wv.length > 0 ? wv[0].path : "";
    }

    function runWindowsInstaller(silent) {
        var wineBin = installWineBinary();
        if (wineBin === "") {
            resetPending();
            noWineDialog.open();
            return;
        }
        pendingPhase = "install";
        var installPrefix = pendingRuntimeType === "proton" ? pendingPrefix + "/pfx" : pendingPrefix;
        gogInstaller.installWindows(pendingGameId, pendingInstallerPath, "wine", wineBin, installPrefix, silent);
    }

    function resolveWindowsRuntime() {
        if (settingsManager.defaultRuntimeType === "wine") {
            var wineBin = settingsManager.defaultWineBinary;
            if (wineBin === "" || !wineScanner.isInstalled(wineBin)) {
                var wv = wineScanner.findWineVersions();
                wineBin = wv.length > 0 ? wv[0].path : "";
            }
            if (wineBin === "") {
                resetPending();
                showPassiveNotification(i18n("No Wine version found. Add one in Settings first."), 6000);
                return false;
            }
            pendingRuntimeType = "wine";
            pendingRuntimePath = wineBin;
            return true;
        }
        var protonPath = settingsManager.defaultProtonPath;
        if (protonPath === "" || !protonScanner.isInstalled(protonPath)) {
            var pv = protonScanner.findProtonVersions();
            protonPath = pv.length > 0 ? pv[0] : "";
        }
        if (protonPath === "") {
            resetPending();
            showPassiveNotification(i18n("No Proton version found. Download GE Proton in Settings first."), 6000);
            return false;
        }
        pendingRuntimeType = "proton";
        pendingRuntimePath = protonPath;
        return true;
    }

    Timer {
        id: searchDebounce
        interval: 350
        onTriggered: gogLibraryModel.fetchLibrary(gogGrid.searchText, 1)
    }

    Connections {
        target: gogLibraryModel
        function onLibraryUpdated(more) {
            gogGrid.hasMore = more;
            if (more)
                Qt.callLater(gogGrid.checkLoadMore);
        }
        function onError(msg) {
            showPassiveNotification(i18n("GOG error: %1", msg), 6000);
        }
        function onInstalledRemoved(gameId) {
            settingsManager.removeGogInstalledGame(gameId);
        }
    }

    Connections {
        target: gogClient
        function onLoginSucceeded() {
            loginDialog.close();
            gogGrid.refresh();
        }
        function onLoginFailed(msg) {
            showPassiveNotification(i18n("GOG login failed: %1", msg), 6000);
        }
        function onAuthenticatedChanged() {
            if (!gogClient.authenticated)
                gogLibraryModel.clear();
        }
        function onError(msg) {
            if (gogGrid.pendingGameId !== "")
                gogGrid.resetPending();
            showPassiveNotification(i18n("GOG error: %1", msg), 6000);
        }
        function onDownloadInfoReady(gameId, urls, isWindows) {
            if (gameId !== gogGrid.pendingGameId)
                return;
            if (!urls || urls.length === 0) {
                gogGrid.resetPending();
                showPassiveNotification(i18n("No installer available for this game"), 5000);
                return;
            }
            gogGrid.pendingIsWindows = isWindows;
            gogGrid.pendingPhase = "download";
            gogDownloader.download(gameId, urls, isWindows);
        }
    }

    Connections {
        target: gogDownloader
        function onDownloadFinished(gameId, primaryPath, isWindows) {
            if (gameId !== gogGrid.pendingGameId)
                return;
            if (primaryPath === "") {
                gogGrid.resetPending();
                showPassiveNotification(i18n("Download produced no installer file"), 5000);
                return;
            }
            gogGrid.pendingInstallerPath = primaryPath;
            gogGrid.pendingPhase = "install";
            if (isWindows) {
                if (!gogGrid.resolveWindowsRuntime())
                    return;
                gogGrid.pendingPrefix = settingsManager.gogInstallDir + "/" + gogGrid.sanitize(gogGrid.pendingGameName);
                if (gogGrid.pendingManual)
                    gogGrid.promptManualInstall();
                else
                    gogGrid.runWindowsInstaller(true);
            } else {
                gogGrid.pendingRuntimeType = "native";
                gogGrid.pendingRuntimePath = "";
                gogGrid.pendingPrefix = settingsManager.gogInstallDir + "/" + gogGrid.sanitize(gogGrid.pendingGameName);
                gogInstaller.installLinux(gameId, primaryPath, gogGrid.pendingPrefix);
            }
        }
        function onDownloadError(gameId, msg) {
            if (gameId !== gogGrid.pendingGameId)
                return;
            gogGrid.resetPending();
            showPassiveNotification(i18n("Download error: %1 (you can retry to resume)", msg), 6000);
        }
    }

    Connections {
        target: gogInstaller
        function onInstallFinished(gameId, exitCode) {
            if (gameId !== gogGrid.pendingGameId)
                return;

            var info = gogGrid.pendingIsWindows ? gogInstaller.findInstalledWindowsGame(gogGrid.pendingPrefix, gogGrid.pendingGameName, gogGrid.pendingRuntimeType) : gogInstaller.findInstalledLinuxGame(gogGrid.pendingPrefix);

            if (!info || info.exePath === undefined || info.exePath === "") {
                // Silent install left nothing behind: fall back to a manual run once.
                if (gogGrid.pendingIsWindows && !gogGrid.pendingManual) {
                    gogGrid.pendingManual = true;
                    gogGrid.promptManualInstall();
                    return;
                }
                showPassiveNotification(i18n("%1 was not installed.", gogGrid.pendingGameName), 7000);
                gogGrid.resetPending();
                return;
            }

            var existing = appModel.getAppByExePath(info.exePath);
            var appId;
            if (existing.id !== undefined) {
                appId = existing.id;
            } else {
                appId = appModel.generateUUID();
                appModel.addApp({
                    "appId": appId,
                    "name": gogGrid.pendingGameName,
                    "exePath": info.exePath,
                    "runtimeType": gogGrid.pendingRuntimeType,
                    "protonPath": gogGrid.pendingRuntimeType === "proton" ? gogGrid.pendingRuntimePath : "",
                    "protonPrefix": gogGrid.pendingRuntimeType === "proton" ? gogGrid.pendingPrefix : "",
                    "wineBinary": gogGrid.pendingRuntimeType === "wine" ? gogGrid.pendingRuntimePath : "",
                    "winePrefix": gogGrid.pendingRuntimeType === "wine" ? gogGrid.pendingPrefix : "",
                    "iconPath": info.iconPath ? info.iconPath : "",
                    "gridPath": "",
                    "heroPath": "",
                    "logoPath": "",
                    "launchOptions": info.launchOptions ? info.launchOptions : "",
                    "enableLogging": false,
                    "steamGridDbId": 0,
                    "steamAppId": 0
                });
            }

            settingsManager.setGogInstalledGame(gameId, info.exePath);
            gogLibraryModel.markInstalled(gameId, info.exePath);
            gogDownloader.clearDownload(gameId);
            showPassiveNotification(i18n("%1 installed", gogGrid.pendingGameName), 4000);

            // Kick off artwork download if configured.
            if (settingsManager.autoDownloadArt && settingsManager.steamGridDbApiKey !== "") {
                gogGrid.artAppId = appId;
                steamGridDb.autoDownloadAll(gogGrid.pendingGameName, protonScanner.localAssetsPath(), settingsManager.steamGridDbApiKey);
            }
            gogGrid.resetPending();
        }
        function onInstallError(gameId, msg) {
            if (gameId !== gogGrid.pendingGameId)
                return;
            gogGrid.resetPending();
            showPassiveNotification(i18n("Install error: %1", msg), 6000);
        }
    }

    Connections {
        target: steamGridDb
        function onAutoDownloadFinished(gameId, iconPath, gridPath, heroPath, logoPath) {
            if (gogGrid.artAppId !== "") {
                appModel.updateAppArt(gogGrid.artAppId, iconPath, gridPath, heroPath, logoPath, gameId);
                gogGrid.artAppId = "";
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !gogClient.authenticated && !gogClient.busy
        text: i18n("Log in to your GOG account to see your library")
        icon.name: "applications-games-symbolic"
        helpfulAction: Kirigami.Action {
            text: i18n("Log in to GOG")
            onTriggered: loginDialog.open()
        }
    }
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: gogClient.authenticated && !gogLibraryModel.busy && gogLibraryModel.count === 0
        text: gogGrid.searchText !== "" ? i18n("No games match your search") : i18n("No games in your GOG library")
        icon.name: "applications-games-symbolic"
    }

    Keys.onReturnPressed: {
        if (gogGrid.activeFocus)
            gogGrid.launchOrInstall(currentIndex);
    }

    onContentYChanged: checkLoadMore()
    onHeightChanged: checkLoadMore()

    delegate: GameCardFrame {
        id: cardFrame
        gv: gogGrid
        displayName: title
        artSource: localCover !== "" ? "file://" + localCover : (coverUrl !== "" ? coverUrl : "")
        iconFallback: ""
        heroLogo: ""
        platformLogo: ""

        required property string gameId
        required property string title
        required property string coverUrl
        required property string localCover
        required property bool worksOnWindows
        required property bool worksOnLinux
        required property bool installed
        required property string exePath
        required property string sizeText

        readonly property bool busyCard: gogGrid.pendingGameId === cardFrame.gameId && gogGrid.pendingPhase !== ""

        Row {
            z: 50
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            anchors.topMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing
            visible: gogGrid.viewType !== "icon" && !cardFrame.busyCard

            Repeater {
                model: {
                    var m = [];
                    if (cardFrame.worksOnWindows)
                        m.push({
                            "label": i18n("Windows"),
                            "color": "#2d6fb0"
                        });
                    if (cardFrame.worksOnLinux)
                        m.push({
                            "label": i18n("Linux"),
                            "color": "#d98c1f"
                        });
                    return m;
                }
                delegate: Rectangle {
                    required property var modelData
                    radius: 3
                    color: modelData.color
                    opacity: 0.92
                    height: badgeLabel.implicitHeight + Kirigami.Units.smallSpacing
                    width: badgeLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                    QQC2.Label {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: modelData.label
                        color: "white"
                        font.pixelSize: 10 * gogGrid.scaleFactor
                        font.bold: true
                    }
                }
            }
        }

        // Download size badge (top-right), shown for games not yet installed.
        Rectangle {
            z: 50
            visible: gogGrid.viewType !== "icon" && !cardFrame.busyCard && !cardFrame.installed && cardFrame.sizeText !== ""
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            anchors.topMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            radius: 3
            color: Qt.rgba(0, 0, 0, 0.6)
            height: sizeLabel.implicitHeight + Kirigami.Units.smallSpacing
            width: sizeLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
            QQC2.Label {
                id: sizeLabel
                anchors.centerIn: parent
                text: cardFrame.sizeText
                color: "white"
                font.pixelSize: 10 * gogGrid.scaleFactor
                font.bold: true
            }
        }

        // Installed / ready-to-play badge (top-right).
        Rectangle {
            z: 50
            visible: gogGrid.viewType !== "icon" && !cardFrame.busyCard && cardFrame.installed
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            anchors.topMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            radius: height / 2
            color: "#3a9e4d"
            width: installedIcon.width + Kirigami.Units.smallSpacing
            height: installedIcon.height + Kirigami.Units.smallSpacing
            Kirigami.Icon {
                id: installedIcon
                anchors.centerIn: parent
                source: "media-playback-start"
                color: "white"
                width: 14 * gogGrid.scaleFactor
                height: 14 * gogGrid.scaleFactor
            }
            QQC2.ToolTip.text: i18n("Installed — double-click to play")
            QQC2.ToolTip.visible: hoverHandler.hovered
            HoverHandler {
                id: hoverHandler
            }
        }

        QQC2.Menu {
            id: gogContextMenu
            QQC2.MenuItem {
                text: cardFrame.installed ? i18n("Play") : i18n("Install")
                icon.name: cardFrame.installed ? "media-playback-start-symbolic" : "folder-download-symbolic"
                enabled: !cardFrame.busyCard && (cardFrame.installed || (!gogDownloader.busy && !gogInstaller.busy && gogGrid.pendingGameId === ""))
                onTriggered: {
                    cardFrame.playLaunchAnimation();
                    gogGrid.launchOrInstall(cardFrame.index);
                }
            }
            QQC2.MenuItem {
                text: i18n("Custom install…")
                icon.name: "system-run-symbolic"
                visible: !cardFrame.installed && cardFrame.worksOnWindows
                height: visible ? implicitHeight : 0
                enabled: !cardFrame.busyCard && !gogDownloader.busy && !gogInstaller.busy && gogGrid.pendingGameId === ""
                onTriggered: gogGrid.startInstall(gogLibraryModel.getGame(cardFrame.index), true)
            }
            QQC2.MenuItem {
                text: i18n("Cancel download")
                icon.name: "dialog-cancel"
                visible: cardFrame.busyCard && gogGrid.pendingPhase === "download"
                height: visible ? implicitHeight : 0
                onTriggered: {
                    gogDownloader.cancel();
                    gogGrid.resetPending();
                }
            }
        }

        // In-cell progress overlay shown while this game is being fetched.
        Rectangle {
            anchors.fill: parent
            visible: cardFrame.busyCard
            radius: Kirigami.Units.cornerRadius
            color: Qt.rgba(0, 0, 0, 0.66)
            z: 100

            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.largeSpacing * 2
                spacing: Kirigami.Units.smallSpacing

                QQC2.ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    indeterminate: gogGrid.pendingPhase !== "download"
                    value: gogDownloader.progress
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: "white"
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    text: {
                        if (gogGrid.pendingPhase === "download")
                            return i18n("Downloading %1%", Math.round(gogDownloader.progress * 100));
                        if (gogGrid.pendingPhase === "install")
                            return i18n("Installing…");
                        return i18n("Preparing…");
                    }
                }
            }
        }

        onLaunched: gogGrid.launchOrInstall(cardFrame.index)
        onContextMenuRequested: gogContextMenu.popup()
    }

    Kirigami.PromptDialog {
        id: manualInstallDialog
        title: i18n("Manual installation")
        subtitle: i18n("%1 will open its installer. Keep the default location (C:\\GOG Games) or use C:\\game so it can be imported automatically.", gogGrid.pendingGameName)
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: gogGrid.doManualInstall()
        onRejected: gogGrid.resetPending()
    }

    Kirigami.PromptDialog {
        id: noWineDialog
        title: i18n("Wine required")
        standardButtons: Kirigami.Dialog.Cancel

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 22
                wrapMode: Text.WordWrap
                text: i18n("Installing GOG games needs Wine, but none was found. Download a build to continue, then start the installation again.")
            }
            QQC2.Button {
                Layout.fillWidth: true
                text: i18n("Download Wine (Wow64)")
                icon.name: "folder-download-symbolic"
                enabled: !wineDownloader.busy
                onClicked: wineDownloader.downloadLatest("wow64")
            }
            QQC2.Button {
                Layout.fillWidth: true
                text: i18n("Download Wine (regular)")
                icon.name: "folder-download-symbolic"
                enabled: !wineDownloader.busy
                onClicked: wineDownloader.downloadLatest("regular")
            }
            QQC2.ProgressBar {
                Layout.fillWidth: true
                visible: wineDownloader.busy
                from: 0
                to: 1
                value: wineDownloader.progress
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: wineDownloader.busy
                wrapMode: Text.WordWrap
                color: Kirigami.Theme.disabledTextColor
                text: wineDownloader.statusText
            }
        }
    }

    Connections {
        target: wineDownloader
        function onFinished(path) {
            if (noWineDialog.visible) {
                noWineDialog.close();
                showPassiveNotification(i18n("Wine downloaded - start the installation again."), 5000);
            }
        }
    }

    Kirigami.PromptDialog {
        id: loginDialog
        title: i18n("Log in to GOG")
        standardButtons: Kirigami.Dialog.NoButton
        customFooterActions: [
            Kirigami.Action {
                text: i18n("Log in")
                enabled: redirectField.text.trim() !== "" && !gogClient.busy
                onTriggered: gogClient.authenticateWithRedirect(redirectField.text)
            }
        ]

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Button {
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 26
                text: i18n("Open GOG login page")
                icon.name: "internet-web-browser"
                onClicked: launcher.openExternalUrl(gogClient.loginUrl())
            }

            QQC2.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: i18n("or if it doesn't open:")
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                color: Kirigami.Theme.disabledTextColor
            }

            QQC2.Button {
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 26
                text: i18n("Copy login URL to clipboard")
                icon.name: "edit-copy-symbolic"
                onClicked: {
                    launcher.copyToClipboard(gogClient.loginUrl());
                    showPassiveNotification(i18n("Login URL copied to clipboard"), 3000);
                }
            }

            QQC2.Label {
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 26
                Layout.topMargin: Kirigami.Units.smallSpacing
                wrapMode: Text.WordWrap
                text: i18n("1. Click the button above and sign in to GOG.\n2. After signing in you'll land on a blank page.\n3. Copy that page's full address from the browser and paste it below.")
            }
            QQC2.TextField {
                id: redirectField
                Layout.fillWidth: true
                placeholderText: i18n("Paste the redirect URL here…")
                onAccepted: if (text.trim() !== "")
                    gogClient.authenticateWithRedirect(text)
            }
        }
    }
}
