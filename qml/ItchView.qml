import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GameGridView {
    id: itchGrid
    model: itchLibraryModel

    property bool hasMore: false
    property string searchText: ""

    // In-flight download/install state (only one at a time). pendingPhase is one
    // of "", "resolve", "download", "install" and drives the in-cell overlay.
    property string pendingGameId: ""
    property string pendingGameName: ""
    property string pendingPhase: ""
    property bool pendingIsWindows: true
    property string pendingDestDir: ""

    function resetPending() {
        pendingGameId = "";
        pendingPhase = "";
    }

    function sanitize(name) {
        return name.replace(/[^a-zA-Z0-9_-]/g, "_");
    }

    function applySearch(text) {
        searchText = text;
        searchDebounce.stop();
        if (itchClient.authenticated)
            searchDebounce.restart();
    }

    function refresh() {
        if (itchClient.authenticated)
            itchLibraryModel.fetchLibrary("", 1);
    }

    function checkLoadMore() {
        if (hasMore && !itchLibraryModel.busy && (contentHeight <= height || contentY + height >= contentHeight - cellHeight * 2))
            itchLibraryModel.fetchNextPage(searchText);
    }

    function launchOrInstall(index) {
        if (index < 0)
            return;
        var g = itchLibraryModel.getGame(index);
        if (g.installed && g.exePath !== "") {
            var app = appModel.getAppByExePath(g.exePath);
            if (app.exePath !== undefined) {
                launcher.launchEntry(app);
                return;
            }
        }
        startInstall(g);
    }

    function startInstall(g) {
        if (itchDownloader.busy || itchInstaller.busy || pendingGameId !== "") {
            showPassiveNotification(i18n("A download or install is already in progress"), 3000);
            return;
        }
        pendingGameId = g.gameId;
        pendingGameName = g.title;
        pendingPhase = "resolve";
        itchClient.fetchDownloadInfo(g.gameId, g.worksOnLinux === true);
    }

    Timer {
        id: searchDebounce
        interval: 350
        onTriggered: itchLibraryModel.fetchLibrary(itchGrid.searchText, 1)
    }

    Connections {
        target: itchLibraryModel
        function onLibraryUpdated(more) {
            itchGrid.hasMore = more;
            if (more)
                Qt.callLater(itchGrid.checkLoadMore);
        }
        function onError(msg) {
            showPassiveNotification(i18n("itch.io error: %1", msg), 6000);
        }
        function onInstalledRemoved(gameId) {
            settingsManager.removeItchInstalledGame(gameId);
        }
    }

    Connections {
        target: itchClient
        function onAuthenticatedChanged() {
            if (itchClient.authenticated)
                itchGrid.refresh();
            else
                itchLibraryModel.clear();
        }
        function onError(msg) {
            if (itchGrid.pendingGameId !== "")
                itchGrid.resetPending();
            showPassiveNotification(i18n("itch.io error: %1", msg), 6000);
        }
        function onDownloadInfoReady(gameId, url, fileName, isWindows) {
            if (gameId !== itchGrid.pendingGameId)
                return;
            if (!url) {
                itchGrid.resetPending();
                showPassiveNotification(i18n("No download available for this game"), 5000);
                return;
            }
            itchGrid.pendingIsWindows = isWindows;
            itchGrid.pendingPhase = "download";
            itchDownloader.download(gameId, url, fileName, isWindows);
        }
    }

    Connections {
        target: itchDownloader
        function onDownloadFinished(gameId, filePath, isWindows) {
            if (gameId !== itchGrid.pendingGameId)
                return;
            if (filePath === "") {
                itchGrid.resetPending();
                showPassiveNotification(i18n("Download produced no file"), 5000);
                return;
            }
            itchGrid.pendingPhase = "install";
            itchGrid.pendingDestDir = settingsManager.itchInstallDir + "/" + itchGrid.sanitize(itchGrid.pendingGameName);
            itchInstaller.extractDownload(gameId, filePath, itchGrid.pendingDestDir);
        }
        function onDownloadError(gameId, msg) {
            if (gameId !== itchGrid.pendingGameId)
                return;
            itchGrid.resetPending();
            showPassiveNotification(i18n("Download error: %1 (you can retry to resume)", msg), 6000);
        }
    }

    Connections {
        target: itchInstaller
        function onInstallFinished(gameId, exitCode) {
            if (gameId !== itchGrid.pendingGameId)
                return;

            var info = itchInstaller.findGame(itchGrid.pendingDestDir, itchGrid.pendingGameName, itchGrid.pendingIsWindows);
            if (!info || info.exePath === undefined || info.exePath === "") {
                showPassiveNotification(i18n("Could not find a runnable file for %1 after download.", itchGrid.pendingGameName), 7000);
                itchGrid.resetPending();
                return;
            }

            var existing = appModel.getAppByExePath(info.exePath);
            if (existing.id === undefined) {
                var safeName = itchGrid.sanitize(itchGrid.pendingGameName).toLowerCase();

                var protonPrefix = "";
                var winePrefix = "";
                if (itchGrid.pendingIsWindows) {
                    protonPrefix = settingsManager.defaultGamePrefix !== "" ? settingsManager.defaultGamePrefix : protonScanner.prefixBasePath() + "/" + safeName;
                    winePrefix = settingsManager.defaultWinePrefix !== "" ? settingsManager.defaultWinePrefix : protonScanner.winePrefixBasePath() + "/" + safeName;
                }

                var libGame = itchLibraryModel.getGameById(gameId);
                var gridPath = "";
                if (libGame && libGame.localCover)
                    gridPath = itchInstaller.copyGridCover(libGame.localCover, protonScanner.localAssetsPath(), safeName);

                appModel.addApp({
                    "appId": appModel.generateUUID(),
                    "name": itchGrid.pendingGameName,
                    "exePath": info.exePath,
                    "runtimeType": itchGrid.pendingIsWindows ? "default" : "native",
                    "protonPath": "",
                    "protonPrefix": protonPrefix,
                    "wineBinary": "",
                    "winePrefix": winePrefix,
                    "iconPath": "",
                    "gridPath": gridPath,
                    "heroPath": "",
                    "logoPath": "",
                    "launchOptions": "",
                    "enableLogging": false,
                    "steamGridDbId": 0,
                    "steamAppId": 0
                });
            }

            settingsManager.setItchInstalledGame(gameId, info.exePath);
            itchLibraryModel.markInstalled(gameId, info.exePath);
            itchDownloader.clearDownload(gameId);
            showPassiveNotification(i18n("%1 installed", itchGrid.pendingGameName), 4000);
            itchGrid.resetPending();
        }
        function onInstallError(gameId, msg) {
            if (gameId !== itchGrid.pendingGameId)
                return;
            itchGrid.resetPending();
            showPassiveNotification(i18n("Install error: %1", msg), 6000);
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !itchClient.authenticated
        text: i18n("Add your itch.io API key in Settings to see your library")
        icon.name: "applications-games-symbolic"
        helpfulAction: Kirigami.Action {
            text: i18n("Open Settings")
            onTriggered: {
                settingsPage.load();
                applicationWindow().navigate("settings");
            }
        }
    }
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: itchClient.authenticated && !itchLibraryModel.busy && itchLibraryModel.count === 0
        text: itchGrid.searchText !== "" ? i18n("No games match your search") : i18n("No games found. Vermouth shows games from your itch.io collections — add some on itch.io first.")
        icon.name: "applications-games-symbolic"
    }

    Keys.onReturnPressed: {
        if (itchGrid.activeFocus)
            itchGrid.launchOrInstall(currentIndex);
    }

    onContentYChanged: checkLoadMore()
    onHeightChanged: checkLoadMore()

    delegate: GameCardFrame {
        id: cardFrame
        gv: itchGrid
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

        readonly property bool busyCard: itchGrid.pendingGameId === cardFrame.gameId && itchGrid.pendingPhase !== ""

        Row {
            z: 50
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            anchors.topMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing
            visible: itchGrid.viewType !== "icon" && !cardFrame.busyCard

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
                        font.pixelSize: 10 * itchGrid.scaleFactor
                        font.bold: true
                    }
                }
            }
        }

        // Installed / ready-to-play badge (top-right).
        Rectangle {
            z: 50
            visible: itchGrid.viewType !== "icon" && !cardFrame.busyCard && cardFrame.installed
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
                width: 14 * itchGrid.scaleFactor
                height: 14 * itchGrid.scaleFactor
            }
            QQC2.ToolTip.text: i18n("Installed — double-click to play")
            QQC2.ToolTip.visible: hoverHandler.hovered
            HoverHandler {
                id: hoverHandler
            }
        }

        QQC2.Menu {
            id: itchContextMenu
            QQC2.MenuItem {
                text: cardFrame.installed ? i18n("Play") : i18n("Install")
                icon.name: cardFrame.installed ? "media-playback-start-symbolic" : "folder-download-symbolic"
                enabled: !cardFrame.busyCard && (cardFrame.installed || (!itchDownloader.busy && !itchInstaller.busy && itchGrid.pendingGameId === ""))
                onTriggered: {
                    cardFrame.playLaunchAnimation();
                    itchGrid.launchOrInstall(cardFrame.index);
                }
            }
            QQC2.MenuItem {
                text: i18n("Cancel download")
                icon.name: "dialog-cancel"
                visible: cardFrame.busyCard && itchGrid.pendingPhase === "download"
                height: visible ? implicitHeight : 0
                onTriggered: {
                    itchDownloader.cancel();
                    itchGrid.resetPending();
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
                    indeterminate: itchGrid.pendingPhase !== "download"
                    value: itchDownloader.progress
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: "white"
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    text: {
                        if (itchGrid.pendingPhase === "download")
                            return i18n("Downloading %1%", Math.round(itchDownloader.progress * 100));
                        if (itchGrid.pendingPhase === "install")
                            return i18n("Installing…");
                        return i18n("Preparing…");
                    }
                }
            }
        }

        onLaunched: itchGrid.launchOrInstall(cardFrame.index)
        onContextMenuRequested: itchContextMenu.popup()
    }
}
