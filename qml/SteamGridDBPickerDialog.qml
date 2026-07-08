import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: i18n("Download from SteamGridDB")
    preferredWidth: Kirigami.Units.gridUnit * 40
    preferredHeight: Kirigami.Units.gridUnit * 30
    standardButtons: Kirigami.Dialog.NoButton
    padding: Kirigami.Units.largeSpacing

    property string gameName: ""
    property string artType: "" // "icon", "grid", "hero", "logo"
    property string apiKey: ""
    property string targetField: "" // to identify which field to fill
    property string selectedGameName: ""

    signal artSelected(string path)
    signal gameIdFound(int id)

    property bool idOnly: false

    function openPicker(name, type, key, fieldId) {
        idOnly = false;
        gameName = name;
        selectedGameName = "";
        artType = type;
        apiKey = key;
        targetField = fieldId;
        gamesModel.clear();
        artModel.clear();
        pickerState = "searching";
        dialog.open();
        steamGridDb.searchGames(name, key);
    }

    function openPickerForId(name, key) {
        idOnly = true;
        gameName = name;
        selectedGameName = "";
        apiKey = key;
        gamesModel.clear();
        artModel.clear();
        pickerState = "searching";
        dialog.open();
        steamGridDb.searchGames(name, key);
    }

    ListModel {
        id: gamesModel
    }
    ListModel {
        id: artModel
    }

    Connections {
        target: steamGridDb
        enabled: dialog.visible
        function onSearchFinished(games) {
            gamesModel.clear();
            for (var i = 0; i < Math.min(games.length, 20); i++) {
                gamesModel.append({
                    id: games[i].id,
                    name: games[i].name,
                    verified: games[i].verified
                });
            }
            if (gamesModel.count === 0) {
                pickerState = "noResults";
            } else if (gamesModel.count === 1) {
                selectGame(0);
            } else {
                pickerState = "games";
            }
        }
        function onGridsFinished(items) {
            populateArt(items);
        }
        function onHeroesFinished(items) {
            populateArt(items);
        }
        function onIconsFinished(items) {
            populateArt(items);
        }
        function onLogosFinished(items) {
            populateArt(items);
        }
        function onDownloadFinished(path) {
            dialog.artSelected(path);
            dialog.close();
        }
        function onError(message) {
            pickerState = "error";
            errorText.text = message;
        }
        function onDownloadProgress(progress) {
            downloadProgress.value = progress;
        }
    }

    function openPickerWithId(gameId, name, type, key, fieldId) {
        idOnly = false;
        gameName = name;
        selectedGameName = "";
        artType = type;
        apiKey = key;
        targetField = fieldId;
        gamesModel.clear();
        artModel.clear();
        pickerState = "fetchingArt";
        dialog.open();
        fetchArtForId(gameId);
    }

    function fetchArtForId(gameId) {
        if (artType === "grid")
            steamGridDb.fetchGrids(gameId, apiKey);
        else if (artType === "hero")
            steamGridDb.fetchHeroes(gameId, apiKey);
        else if (artType === "logo")
            steamGridDb.fetchLogos(gameId, apiKey);
        else
            steamGridDb.fetchIcons(gameId, apiKey);
    }

    function selectGame(index) {
        var gameId = gamesModel.get(index).id;
        selectedGameName = gamesModel.get(index).name;
        gameIdFound(gameId);
        if (idOnly) {
            dialog.close();
            return;
        }
        pickerState = "fetchingArt";
        fetchArtForId(gameId);
    }

    function populateArt(items) {
        artModel.clear();
        for (var i = 0; i < Math.min(items.length, 20); i++) {
            artModel.append({
                id: items[i].id,
                url: items[i].url,
                thumb: items[i].thumb,
                score: items[i].score,
                style: items[i].style,
                author: items[i].author
            });
        }
        if (artModel.count === 0) {
            pickerState = "noArt";
        } else {
            pickerState = "art";
        }
    }

    function downloadArt(index) {
        var item = artModel.get(index);
        var url = item.url;
        var ext = url.substring(url.lastIndexOf(".") + 1);
        if (ext.indexOf("?") >= 0)
            ext = ext.substring(0, ext.indexOf("?"));
        if (ext.length === 0)
            ext = "png";
        var safeName = gameName.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase();
        var fileName = safeName + "_" + artType + "_" + item.id + "." + ext;
        var savePath = protonScanner.localAssetsPath() + "/" + fileName;
        pickerState = "downloading";
        downloadProgress.value = 0;
        steamGridDb.downloadImage(url, savePath);
    }

    property string pickerState: "searching"

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing
        width: parent.width

        QQC2.Label {
            text: {
                if (dialog.pickerState === "searching")
                    return i18n("Searching for '%1'…", dialog.gameName);
                if (dialog.pickerState === "games")
                    return i18n("Select a game:");
                if (dialog.pickerState === "fetchingArt")
                    return i18n("Loading %1 art…", dialog.artType);
                if (dialog.pickerState === "art")
                    return i18n("Select a %1 image for %2: (%3 results)", dialog.artType, dialog.selectedGameName || dialog.gameName, artModel.count);
                if (dialog.pickerState === "downloading")
                    return i18n("Downloading…");
                if (dialog.pickerState === "noResults")
                    return i18n("No games found for '%1'.", dialog.gameName);
                if (dialog.pickerState === "noArt")
                    return i18n("No %1 art found for this game.", dialog.artType);
                if (dialog.pickerState === "error")
                    return i18n("Error:");
                return "";
            }
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        QQC2.BusyIndicator {
            visible: dialog.pickerState === "searching" || dialog.pickerState === "fetchingArt"
            running: visible
            Layout.alignment: Qt.AlignHCenter
        }

        QQC2.ProgressBar {
            id: downloadProgress
            visible: dialog.pickerState === "downloading"
            Layout.fillWidth: true
            from: 0
            to: 1
        }

        QQC2.Label {
            id: errorText
            visible: dialog.pickerState === "error"
            color: Kirigami.Theme.negativeTextColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Game selection list
        ListView {
            id: gamesList
            visible: dialog.pickerState === "games"
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 18
            model: gamesModel
            clip: true
            spacing: Kirigami.Units.smallSpacing

            delegate: Rectangle {
                width: gamesList.width
                height: Kirigami.Units.gridUnit * 5
                radius: Kirigami.Units.cornerRadius
                color: mouseArea.containsMouse ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor
                border.color: Kirigami.Theme.disabledTextColor
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        text: model.name
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        color: mouseArea.containsMouse ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                    }
                    QQC2.Label {
                        text: model.verified ? i18n("✓ Verified") : i18n("Unverified")
                        font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                        color: model.verified ? (mouseArea.containsMouse ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.positiveTextColor) : (mouseArea.containsMouse ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.disabledTextColor)
                    }
                    QQC2.Label {
                        text: "ID: " + model.id
                        font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                        opacity: 0.6
                        color: mouseArea.containsMouse ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: dialog.selectGame(index)
                }
            }
        }

        // Art selection grid
        GridView {
            id: artGrid
            visible: dialog.pickerState === "art"
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 22
            cellWidth: Math.floor(width / 3)
            cellHeight: artType === "hero" ? cellWidth * 0.46 : (artType === "icon" || artType === "logo") ? cellWidth : cellWidth * 1.5
            model: artModel
            clip: true

            delegate: Rectangle {
                width: artGrid.cellWidth - Kirigami.Units.smallSpacing
                height: artGrid.cellHeight - Kirigami.Units.smallSpacing
                radius: Kirigami.Units.cornerRadius
                color: Kirigami.Theme.backgroundColor
                border.color: artMouseArea.containsMouse ? Kirigami.Theme.highlightColor : Kirigami.Theme.disabledTextColor
                border.width: artMouseArea.containsMouse ? 2 : 1
                layer.enabled: true

                Image {
                    anchors.fill: parent
                    source: model.thumb
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: scoreLabel.height + Kirigami.Units.smallSpacing * 2
                    color: Qt.rgba(0, 0, 0, 0.6)

                    QQC2.Label {
                        id: scoreLabel
                        anchors.centerIn: parent
                        text: "Score: " + model.score + (model.style ? " · " + model.style : "")
                        color: "white"
                        font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    }
                }

                MouseArea {
                    id: artMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: dialog.downloadArt(index)
                }
            }
        }

        RowLayout {
            visible: dialog.pickerState === "games" || dialog.pickerState === "art"
            Layout.alignment: Qt.AlignHCenter

            QQC2.Button {
                visible: dialog.pickerState === "art" && gamesModel.count > 0
                text: i18n("Back")
                icon.name: "go-previous"
                onClicked: {
                    artModel.clear();
                    dialog.pickerState = gamesModel.count === 1 ? "searching" : "games";
                    if (gamesModel.count === 1) {
                        gamesModel.clear();
                        steamGridDb.searchGames(dialog.gameName, dialog.apiKey);
                    }
                }
            }

            QQC2.Button {
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onClicked: dialog.close()
            }
        }

        QQC2.Button {
            visible: dialog.pickerState === "noResults" || dialog.pickerState === "noArt" || dialog.pickerState === "error"
            text: i18n("Close")
            icon.name: "window-close-symbolic"
            Layout.alignment: Qt.AlignHCenter
            onClicked: dialog.close()
        }
    }
}
