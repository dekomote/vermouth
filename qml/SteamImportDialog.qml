import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: i18n("Import from Steam")
    preferredWidth: Kirigami.Units.gridUnit * 35
    padding: Kirigami.Units.largeSpacing
    bottomPadding: 30
    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: [
        Kirigami.Action {
            text: i18n("Import Selected")
            icon.name: "list-add"
            enabled: selectedCount > 0 && !importing
            onTriggered: doImport()
        },
        Kirigami.Action {
            text: i18n("Close")
            icon.name: "dialog-close"
            onTriggered: dialog.close()
        }
    ]

    property int selectedCount: 0
    property bool importing: false
    property var checkStates: ({})
    property var downloadQueue: []
    property string currentDownloadAppId: ""
    property string autoDownloadStatus: ""

    function openDialog() {
        checkStates = {};
        selectedCount = 0;
        importing = false;
        searchField.text = "";
        downloadQueue = [];
        currentDownloadAppId = "";
        autoDownloadStatus = "";
        if (steamModel.count === 0 && !steamModel.busy)
            steamModel.scanLibraries();
        dialog.open();
    }

    function doImport() {
        importing = true;
        for (var i = 0; i < steamModel.count; i++) {
            if (checkStates[i]) {
                var g = steamModel.getGame(i);
                if (appModel.hasSteamApp(g.steamId))
                    continue;
                var appId = generateUUID();
                var missingArt = g.gridPath === "" || g.heroPath === "" || g.logoPath === "";
                appModel.addApp({
                    "appId": appId,
                    "name": g.name,
                    "exePath": "",
                    "runtimeType": "steam",
                    "steamAppId": g.steamId,
                    "protonPath": "",
                    "protonPrefix": "",
                    "wineBinary": "",
                    "winePrefix": "",
                    "iconPath": g.iconPath,
                    "gridPath": g.gridPath,
                    "heroPath": g.heroPath,
                    "logoPath": g.logoPath,
                    "launchOptions": "",
                    "enableLogging": false,
                    "steamGridDbId": 0
                });
                if (missingArt && settingsManager.autoDownloadArt && settingsManager.steamGridDbApiKey !== "")
                    downloadQueue.push({
                        "appId": appId,
                        "name": g.name
                    });
            }
        }
        if (downloadQueue.length > 0) {
            autoDownloadStatus = i18n("Downloading artwork…");
            startNextDownload();
        } else {
            dialog.close();
        }
    }

    function generateUUID() {
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
            var r = Math.random() * 16 | 0, v = c === 'x' ? r : (r & 0x3 | 0x8);
            return v.toString(16);
        });
    }

    function startNextDownload() {
        if (downloadQueue.length === 0) {
            dialog.close();
            return;
        }
        var item = downloadQueue.shift();
        currentDownloadAppId = item.appId;
        autoDownloadStatus = i18n("Downloading artwork for %1…", item.name);
        steamGridDb.autoDownloadAll(item.name, protonScanner.localAssetsPath(), settingsManager.steamGridDbApiKey);
    }

    Connections {
        target: steamGridDb
        function onAutoDownloadFinished(gameId, iconPath, gridPath, heroPath, logoPath) {
            if (currentDownloadAppId !== "") {
                appModel.updateAppArt(currentDownloadAppId, iconPath, gridPath, heroPath, logoPath, gameId);
                currentDownloadAppId = "";
            }
            startNextDownload();
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing
        anchors.fill: parent

        Kirigami.SearchField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: i18n("Search Steam games…")
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: autoDownloadStatus !== "" ? autoDownloadStatus : steamModel.busy ? (steamModel.statusText || i18n("Scanning…")) : selectedCount > 0 ? i18np("%1 game selected", "%1 games selected", selectedCount) : i18np("%1 game found", "%1 games found", steamModel.count)
            opacity: 0.75
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: gameList
                width: parent.width
                height: parent.height
                model: steamModel
                spacing: 0

                delegate: Item {
                    id: rowItem
                    width: gameList.width
                    height: visible ? Math.max(54, rowContent.implicitHeight) : 0

                    required property int index
                    required property int steamId
                    required property string name
                    required property string gridPath
                    required property string heroPath
                    required property string iconPath
                    required property string logoPath
                    required property string installDir

                    property bool matchesFilter: searchField.text === "" || name.toLowerCase().includes(searchField.text.toLowerCase())
                    visible: matchesFilter

                    Rectangle {
                        anchors.fill: rowContent
                        color: index % 2 === 0 ? "transparent" : Kirigami.Theme.alternateBackgroundColor
                        opacity: 0.4
                    }

                    RowLayout {
                        id: rowContent
                        width: parent.width
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.CheckBox {
                            id: cb
                            checked: dialog.checkStates[index] === true
                            enabled: appModel ? !appModel.hasSteamApp(steamId) : true
                            onCheckedChanged: {
                                dialog.checkStates[index] = checked;
                                dialog.selectedCount = Object.values(dialog.checkStates).filter(function (v) {
                                    return v;
                                }).length;
                            }
                        }

                        Image {
                            Layout.preferredWidth: 38
                            Layout.preferredHeight: 54
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            source: iconPath !== "" ? "file://" + iconPath : (gridPath !== "" ? "file://" + gridPath : "")
                            visible: source !== ""
                            sourceSize: Qt.size(76, 108)
                        }

                        Kirigami.Heading {
                            text: name
                            level: 5
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            if (cb.enabled) {
                                cb.checked = !cb.checked;
                                dialog.checkStates[index] = cb.checked;
                                dialog.selectedCount = Object.values(dialog.checkStates).filter(function (v) {
                                    return v;
                                }).length;
                            }
                        }
                    }
                }
            }
        }
    }
}
