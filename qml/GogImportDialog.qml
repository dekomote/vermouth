import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: i18n("Import from GOG")
    preferredWidth: Kirigami.Units.gridUnit * 30
    padding: Kirigami.Units.largeSpacing
    bottomPadding: Kirigami.Units.largeSpacing
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
        dialog.open();
    }

    function doImport() {
        importing = true;
        var protonVersions = protonScanner.findProtonVersions();
        var defaultProton = protonVersions.length > 0 ? protonVersions[0] : "";
        var prefixBase = protonScanner.prefixBasePath();

        for (var i = 0; i < gogModel.count; i++) {
            if (!checkStates[i])
                continue;
            var g = gogModel.getGame(i);
            if (appModel.getAppByExePath(g.exePath).id !== undefined)
                continue;
            var appId = generateUUID();
            var protonPrefix = g.isWindows ? prefixBase + "/" + g.name.replace(/[^a-zA-Z0-9_-]/g, "_") : "";
            appModel.addApp({
                "appId": appId,
                "name": g.name,
                "exePath": g.exePath,
                "runtimeType": g.isWindows ? "proton" : "native",
                "protonPath": g.isWindows ? defaultProton : "",
                "protonPrefix": protonPrefix,
                "wineBinary": "",
                "winePrefix": "",
                "iconPath": g.iconPath,
                "gridPath": "",
                "heroPath": "",
                "logoPath": "",
                "launchOptions": "",
                "enableLogging": false,
                "steamGridDbId": 0,
                "steamAppId": 0
            });
            if (settingsManager.autoDownloadArt && settingsManager.steamGridDbApiKey !== "")
                downloadQueue.push({
                    "appId": appId,
                    "name": g.name
                });
        }
        if (downloadQueue.length > 0) {
            autoDownloadStatus = i18n("Downloading artwork…");
            startNextDownload();
        } else {
            dialog.close();
        }
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

    function generateUUID() {
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
            var r = Math.random() * 16 | 0, v = c === 'x' ? r : (r & 0x3 | 0x8);
            return v.toString(16);
        });
    }

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing
        Layout.fillWidth: true

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.TextField {
                id: folderField
                Layout.fillWidth: true
                placeholderText: i18n("GOG games folder…")
                onTextChanged: {
                    if (text.length > 0 && !gogModel.busy) {
                        checkStates = {};
                        selectedCount = 0;
                        gogModel.scanFolder(text);
                    }
                }
            }

            QQC2.Button {
                icon.name: "folder-open"
                onClicked: folderDialog.open()
            }
        }

        Kirigami.SearchField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: i18n("Search GOG games…")
            visible: gogModel.count > 0 || gogModel.busy
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: folderField.text.length > 0
            text: autoDownloadStatus !== "" ? autoDownloadStatus : gogModel.busy ? i18n("Scanning…") : selectedCount > 0 ? i18np("%1 game selected", "%1 games selected", selectedCount) : i18np("%1 game found", "%1 games found", gogModel.count)
            opacity: 0.75
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: gogModel.count > 0

            ListView {
                id: gameList
                width: parent.width
                height: parent.height
                model: gogModel
                spacing: 0

                delegate: Item {
                    id: rowItem
                    width: gameList.width
                    height: visible ? Math.max(54, rowContent.implicitHeight) : 0

                    required property int index
                    required property string gameId
                    required property string name
                    required property string exePath
                    required property string iconPath
                    required property bool isWindows

                    property bool alreadyImported: appModel.getAppByExePath(exePath).id !== undefined
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
                            checkable: false
                            checked: dialog.checkStates[index] === true
                            enabled: !alreadyImported
                        }

                        Kirigami.Heading {
                            text: name
                            level: 5
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        QQC2.Label {
                            text: alreadyImported ? i18n("Imported") : isWindows ? i18n("Windows") : i18n("Linux")
                            opacity: 0.6
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            if (alreadyImported)
                                return;
                            var s = Object.assign({}, dialog.checkStates);
                            s[index] = !(s[index] === true);
                            dialog.checkStates = s;
                            dialog.selectedCount = Object.values(dialog.checkStates).filter(function (v) {
                                return v;
                            }).length;
                        }
                    }
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: i18n("Select GOG Games Folder")
        onAccepted: folderField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }
}
