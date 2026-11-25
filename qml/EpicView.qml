import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Epic Games Store library, backed by the `legendary` command line tool.
GameGridView {
    id: epicGrid
    model: epicLibraryModel

    signal gameStarted(string appId)

    property string searchText: ""
    property string pendingUninstall: ""
    property string pendingUpdateName: ""
    property string updatingApp: ""

    function startUpdate(appName) {
        if (epicClient.installingApp !== "") {
            showPassiveNotification(i18n("An install is already in progress"), 3000);
            return;
        }
        updatingApp = appName;
        epicClient.update(appName);
    }

    function applySearch(text) {
        searchText = text;
        epicLibraryModel.setFilter(text);
    }

    function openLogin() {
        loginDialog.open();
    }

    function refresh() {
        if (epicClient.authenticated)
            epicLibraryModel.refresh();
    }

    property string pendingAdd: ""
    property string artAppId: ""

    // Adds the installed game to the main list (default runtime) and returns its entry.
    function ensureEntry(g) {
        var existing = appModel.getAppByExePath(g.exePath);
        if (existing.exePath !== undefined)
            return existing;
        // Same rule as the Add Game dialog: the default prefix if one is set, else a folder per game in the prefix base path.
        var safeName = g.title.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase();
        var protonPrefix = settingsManager.defaultGamePrefix !== "" ? settingsManager.defaultGamePrefix : protonScanner.prefixBasePath() + "/" + safeName;
        var winePrefix = settingsManager.defaultWinePrefix !== "" ? settingsManager.defaultWinePrefix : protonScanner.winePrefixBasePath() + "/" + safeName;
        var appId = appModel.generateUUID();
        appModel.addApp({
            "appId": appId,
            "name": g.title,
            "exePath": g.exePath,
            "runtimeType": "default",
            "epicAppName": g.appName,
            "protonPath": "",
            "protonPrefix": protonPrefix,
            "wineBinary": "",
            "winePrefix": winePrefix,
            "iconPath": "",
            "gridPath": g.coverPath ? g.coverPath : "",
            "heroPath": "",
            "logoPath": "",
            "launchOptions": "",
            "enableLogging": false,
            "steamGridDbId": 0,
            "steamAppId": 0
        });
        if (settingsManager.autoDownloadArt && settingsManager.steamGridDbApiKey !== "") {
            artAppId = appId;
            steamGridDb.autoDownloadAll(g.title, protonScanner.localAssetsPath(), settingsManager.steamGridDbApiKey);
        }
        return appModel.getAppByExePath(g.exePath);
    }

    function playOrInstall(index) {
        if (index < 0)
            return;
        var g = epicLibraryModel.getGame(index);
        if (g.installed) {
            if (g.updateAvailable) {
                pendingUpdateName = g.appName;
                updateDialog.subtitle = i18n("%1 has an update, and Epic won't let an outdated game start. Update it now?", g.title);
                updateDialog.open();
                return;
            }
            if (g.exePath === "") {
                showPassiveNotification(i18n("Could not find the game's executable"), 5000);
                return;
            }
            var app = ensureEntry(g);
            if (app.exePath !== undefined) {
                launcher.launchEntry(app);
                gameStarted(app.id);
            }
            return;
        }
        if (epicClient.installingApp !== "") {
            showPassiveNotification(i18n("An install is already in progress"), 3000);
            return;
        }
        epicClient.install(g.appName, settingsManager.epicInstallDir);
    }

    Connections {
        target: epicLibraryModel
        function onLibraryUpdated() {
            if (epicGrid.pendingAdd === "")
                return;
            var name = epicGrid.pendingAdd;
            epicGrid.pendingAdd = "";
            for (var i = 0; i < epicLibraryModel.count; i++) {
                var g = epicLibraryModel.getGame(i);
                if (g.appName === name && g.installed && g.exePath !== "") {
                    epicGrid.ensureEntry(g);
                    showPassiveNotification(i18n("%1 installed and added to your games", g.title), 5000);
                    return;
                }
            }
            showPassiveNotification(i18n("Installed, but the game's executable was not found"), 6000);
        }
    }

    Connections {
        target: steamGridDb
        function onAutoDownloadFinished(gameId, iconPath, gridPath, heroPath, logoPath) {
            if (epicGrid.artAppId !== "") {
                appModel.updateAppArt(epicGrid.artAppId, iconPath, gridPath, heroPath, logoPath, gameId);
                epicGrid.artAppId = "";
            }
        }
    }

    Connections {
        target: epicClient
        function onLoginSucceeded() {
            loginDialog.close();
            epicGrid.refresh();
        }
        function onLoginFailed(msg) {
            showPassiveNotification(i18n("Epic login failed: %1", msg), 6000);
        }
        function onAuthenticatedChanged() {
            if (epicClient.authenticated)
                epicGrid.refresh();
        }
        function onError(msg) {
            showPassiveNotification(i18n("Epic error: %1", msg), 6000);
        }
        function onInstallFinished(appName, ok, message) {
            if (ok && appName === epicGrid.updatingApp) {
                epicGrid.updatingApp = "";
                epicLibraryModel.refresh();
                showPassiveNotification(i18n("Updated"), 4000);
            } else if (ok) {
                epicGrid.pendingAdd = appName;
                epicLibraryModel.refresh();
            } else {
                epicGrid.updatingApp = "";
                showPassiveNotification(i18n("Install failed: %1", message), 7000);
            }
        }
        function onUninstalled(appName) {
            epicLibraryModel.markUninstalled(appName);
            showPassiveNotification(i18n("Uninstalled"), 3000);
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !epicClient.authenticated && !epicClient.busy
        text: i18n("Log in to your Epic Games account to see your library")
        icon.name: "applications-games-symbolic"
        helpfulAction: Kirigami.Action {
            text: i18n("Log in to Epic Games")
            onTriggered: loginDialog.open()
        }
    }
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: epicClient.authenticated && !epicClient.busy && epicLibraryModel.count === 0
        text: epicGrid.searchText !== "" ? i18n("No games match your search") : i18n("No games in your Epic library")
        icon.name: "applications-games-symbolic"
    }

    Keys.onReturnPressed: {
        if (epicGrid.activeFocus)
            epicGrid.playOrInstall(currentIndex);
    }

    delegate: GameCardFrame {
        id: cardFrame
        gv: epicGrid
        displayName: title
        artSource: localCover !== "" ? "file://" + localCover : (coverUrl !== "" ? coverUrl : "")
        iconFallback: ""
        heroLogo: ""
        platformLogo: ""

        required property string appName
        required property string title
        required property string coverUrl
        required property string localCover
        required property bool installed
        required property string installPath
        required property bool updateAvailable

        readonly property bool installing: epicClient.installingApp === cardFrame.appName

        Rectangle {
            z: 50
            visible: epicGrid.viewType !== "icon" && !cardFrame.installing && cardFrame.installed
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
                width: 14 * epicGrid.scaleFactor
                height: 14 * epicGrid.scaleFactor
            }
        }

        // Update button (top-right) for installed games that have a newer version.
        Rectangle {
            z: 60
            visible: epicGrid.viewType !== "icon" && !cardFrame.installing && cardFrame.installed && cardFrame.updateAvailable
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            anchors.topMargin: Kirigami.Units.mediumSpacing + Kirigami.Units.smallSpacing
            radius: height / 2
            color: updateArea.containsMouse ? "#f0a030" : "#d98c1f"
            width: updateRow.implicitWidth + Kirigami.Units.smallSpacing * 2
            height: updateRow.implicitHeight + Kirigami.Units.smallSpacing
            Row {
                id: updateRow
                anchors.centerIn: parent
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    source: "folder-download-symbolic"
                    color: "white"
                    width: 14 * epicGrid.scaleFactor
                    height: 14 * epicGrid.scaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                }
                QQC2.Label {
                    text: i18n("Update")
                    color: "white"
                    font.pixelSize: 10 * epicGrid.scaleFactor
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                id: updateArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                enabled: epicClient.installingApp === ""
                onClicked: epicGrid.startUpdate(cardFrame.appName)
            }
        }

        QQC2.Menu {
            id: epicContextMenu
            QQC2.MenuItem {
                text: cardFrame.installed ? i18n("Play") : i18n("Install")
                icon.name: cardFrame.installed ? "media-playback-start-symbolic" : "folder-download-symbolic"
                enabled: !cardFrame.installing && (cardFrame.installed || epicClient.installingApp === "")
                onTriggered: {
                    cardFrame.playLaunchAnimation();
                    epicGrid.playOrInstall(cardFrame.index);
                }
            }
            QQC2.MenuItem {
                text: i18n("Update")
                icon.name: "folder-download-symbolic"
                visible: cardFrame.installed && cardFrame.updateAvailable
                height: visible ? implicitHeight : 0
                enabled: epicClient.installingApp === ""
                onTriggered: epicGrid.startUpdate(cardFrame.appName)
            }
            QQC2.MenuItem {
                text: i18n("Cancel install")
                icon.name: "dialog-cancel"
                visible: cardFrame.installing
                height: visible ? implicitHeight : 0
                onTriggered: epicClient.cancelInstall()
            }
            QQC2.MenuItem {
                text: i18n("Open install folder")
                icon.name: "folder-open-symbolic"
                visible: cardFrame.installed
                height: visible ? implicitHeight : 0
                onTriggered: Qt.openUrlExternally("file://" + cardFrame.installPath)
            }
            QQC2.MenuItem {
                text: i18n("Uninstall…")
                icon.name: "edit-delete-symbolic"
                visible: cardFrame.installed
                height: visible ? implicitHeight : 0
                onTriggered: {
                    epicGrid.pendingUninstall = cardFrame.appName;
                    uninstallDialog.subtitle = i18n("Remove %1 and its files?", cardFrame.title);
                    uninstallDialog.open();
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: cardFrame.installing
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
                    indeterminate: epicClient.progress <= 0
                    value: epicClient.progress
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: "white"
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    text: epicClient.statusText !== "" ? epicClient.statusText : i18n("Preparing…")
                }
            }
        }

        onLaunched: epicGrid.playOrInstall(cardFrame.index)
        onContextMenuRequested: epicContextMenu.popup()
    }

    Kirigami.PromptDialog {
        id: updateDialog
        title: i18n("Update available")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: epicGrid.startUpdate(epicGrid.pendingUpdateName)
    }

    Kirigami.PromptDialog {
        id: uninstallDialog
        title: i18n("Uninstall game")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: epicClient.uninstall(epicGrid.pendingUninstall)
    }

    Kirigami.PromptDialog {
        id: loginDialog
        title: i18n("Log in to Epic Games")
        standardButtons: Kirigami.Dialog.NoButton
        customFooterActions: [
            Kirigami.Action {
                text: i18n("Log in")
                enabled: codeField.text.trim() !== "" && !epicClient.busy
                onTriggered: epicClient.authenticateWithCode(codeField.text)
            }
        ]

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Button {
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 26
                text: i18n("Open Epic login page")
                icon.name: "internet-web-browser"
                onClicked: launcher.openExternalUrl(epicClient.loginUrl())
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
                    launcher.copyToClipboard(epicClient.loginUrl());
                    showPassiveNotification(i18n("Login URL copied to clipboard"), 3000);
                }
            }
            QQC2.Label {
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 26
                Layout.topMargin: Kirigami.Units.smallSpacing
                wrapMode: Text.WordWrap
                text: i18n("1. Click the button above and sign in to Epic Games.\n2. The page then shows a short block of text containing an authorizationCode.\n3. Copy that whole text (or just the code) and paste it below.")
            }
            QQC2.TextField {
                id: codeField
                Layout.fillWidth: true
                placeholderText: i18n("Paste the authorization code here…")
                onAccepted: if (text.trim() !== "")
                    epicClient.authenticateWithCode(text)
            }
        }
    }
}
