import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GameGridView {
    id: romGrid
    model: rommModel

    property var platforms: []
    property var platformLogoMap: ({})
    property bool hasMore: false
    property int currentPlatformId: -1
    property string searchText: ""

    function applySearch(text) {
        searchText = text;
        searchDebounce.stop();
        if (currentPlatformId >= 0)
            searchDebounce.restart();
    }

    function refresh() {
        rommModel.fetchPlatforms();
    }

    function checkLoadMore() {
        if (hasMore && !rommModel.busy && (contentHeight <= height || contentY + height >= contentHeight - cellHeight * 2)) {
            var nextPage = Math.floor(rommModel.count / 50) + 1;
            rommModel.fetchRoms(currentPlatformId, searchText, nextPage);
        }
    }

    function launchOrDownload(index, enableLogging) {
        if (index < 0)
            return;
        var logging = enableLogging === true;
        var rom = rommModel.getRom(index);
        var files = rom.fileNames;
        if (!files || files.length === 0)
            return;

        if (files.length === 1) {
            var cached = rommFileDownloader.cachedRomPath(rom.romId, files[0]);
            if (cached !== "") {
                rom.localRomPath = cached;
                launcher.launchRom(rom, logging);
            } else {
                startDownload(rom, files[0]);
            }
        } else {
            for (var i = 0; i < files.length; i++) {
                var c = rommFileDownloader.cachedRomPath(rom.romId, files[i]);
                if (c !== "") {
                    rom.localRomPath = c;
                    launcher.launchRom(rom, logging);
                    return;
                }
            }
            openFilePicker(rom);
        }
    }

    function openFilePicker(rom) {
        filePickerDialog.pendingRom = rom;
        filePickerDialog.open();
    }

    function startDownload(rom, fileName) {
        if (rommFileDownloader.busy) {
            showPassiveNotification(i18n("A download is already in progress"), 3000);
            return;
        }
        downloadProgressDialog.pendingRom = rom;
        downloadProgressDialog.open();
        rommFileDownloader.downloadRom(rom.romId, fileName);
    }

    Connections {
        target: rommModel
        function onPlatformsFetched(p) {
            var logoMap = {};
            for (var i = 0; i < p.length; i++) {
                if (p[i].logoUrl)
                    logoMap[p[i].slug] = p[i].logoUrl;
            }
            romGrid.platformLogoMap = logoMap;
            romGrid.platforms = [
                {
                    id: 0,
                    name: i18n("All Platforms")
                }
            ].concat(p);
        }
        function onRomsFetched(more) {
            romGrid.hasMore = more;
            if (more)
                Qt.callLater(romGrid.checkLoadMore);
        }
        function onError(msg) {
            showPassiveNotification(i18n("RomM error: %1", msg), 6000);
        }
    }

    Connections {
        target: launcher
        function onRomCoreMissing(platformSlug, rom) {
            mainCorePicker.platformSlug = platformSlug;
            mainCorePicker.pendingRom = rom;
            mainCorePicker.appIndex = -1;
            mainCorePicker.launchAfterPick = true;
            mainCorePicker.open();
        }
    }

    Connections {
        target: rommFileDownloader
        function onRomDownloaded(romId, localPath) {
            downloadProgressDialog.close();
            var rom = downloadProgressDialog.pendingRom;
            if (rom && rom.romId === romId) {
                rom.localRomPath = localPath;
                launcher.launchRom(rom);
            }
        }
        function onDownloadError(romId, message) {
            downloadProgressDialog.close();
            showPassiveNotification(i18n("Download error: %1", message), 6000);
        }
    }

    Timer {
        id: searchDebounce
        interval: 350
        onTriggered: rommModel.fetchRoms(romGrid.currentPlatformId, romGrid.searchText)
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !rommModel.busy && rommModel.count === 0 && romGrid.currentPlatformId >= 0
        text: i18n("No ROMs found")
        icon.name: "folder-games"
    }
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !rommModel.busy && romGrid.currentPlatformId === -1 && romGrid.platforms.length > 0
        text: i18n("Select a platform")
        icon.name: "folder-games"
    }
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !rommModel.busy && romGrid.platforms.length === 0
        text: settingsManager.rommServerUrl !== "" ? i18n("No platforms") : i18n("Configure RomM\nin Settings")
        icon.name: settingsManager.rommServerUrl !== "" ? "folder-games" : "configure"
    }

    Keys.onReturnPressed: {
        if (romGrid.activeFocus)
            romGrid.launchOrDownload(currentIndex);
    }

    onContentYChanged: checkLoadMore()
    onHeightChanged: checkLoadMore()

    delegate: GameCardFrame {
        id: cardFrame
        gv: romGrid
        displayName: name
        artSource: localCover !== "" ? "file://" + localCover : (coverUrl !== "" ? coverUrl : "")
        iconFallback: ""
        heroLogo: ""
        platformLogo: romGrid.platformLogoMap[platformSlug] ?? ""

        required property int romId
        required property string name
        required property string fileName
        required property var fileNames
        required property string platformSlug
        required property string coverUrl
        required property string localCover
        required property double fileSizeBytes

        QQC2.Menu {
            id: romContextMenu
            QQC2.MenuItem {
                text: i18n("Launch")
                icon.name: "media-playback-start"
                onTriggered: {
                    cardFrame.playLaunchAnimation();
                    romGrid.launchOrDownload(cardFrame.index);
                }
            }
            QQC2.MenuItem {
                text: i18n("Launch with Log")
                icon.name: "text-x-log"
                onTriggered: {
                    cardFrame.playLaunchAnimation();
                    romGrid.launchOrDownload(cardFrame.index, true);
                }
            }
            QQC2.MenuItem {
                readonly property string _firstCached: rommFileDownloader.cachedRomPath(cardFrame.romId, cardFrame.fileName)
                text: _firstCached !== "" ? i18n("ROM cached locally") : i18n("Download ROM (%1 MB)").arg((cardFrame.fileSizeBytes / (1024 * 1024)).toFixed(1))
                icon.name: "download"
                enabled: _firstCached === "" && !rommFileDownloader.busy
                onTriggered: {
                    var rom = rommModel.getRom(cardFrame.index);
                    var files = rom.fileNames;
                    if (!files || files.length <= 1)
                        romGrid.startDownload(rom, files ? files[0] : rom.fileName);
                    else
                        romGrid.openFilePicker(rom);
                }
            }
            QQC2.MenuItem {
                text: i18n("Change Core…")
                icon.name: "media-record"
                onTriggered: {
                    var rom = rommModel.getRom(cardFrame.index);
                    mainCorePicker.platformSlug = cardFrame.platformSlug;
                    mainCorePicker.pendingRom = rom;
                    mainCorePicker.appIndex = -1;
                    mainCorePicker.launchAfterPick = false;
                    mainCorePicker.open();
                }
            }
            QQC2.MenuItem {
                text: i18n("Open log folder")
                icon.name: "folder-open"
                onTriggered: Qt.openUrlExternally("file://" + launcher.logDir())
            }
            QQC2.MenuItem {
                text: i18n("Copy Launch Command")
                icon.name: "edit-copy"
                onTriggered: {
                    var rom = rommModel.getRom(cardFrame.index);
                    var cached = rommFileDownloader.cachedRomPath(cardFrame.romId, cardFrame.fileName);
                    if (cached !== "")
                        rom.localRomPath = cached;
                    var cmd = launcher.buildRomLaunchCommand(rom);
                    if (cmd !== "") {
                        launcher.copyToClipboard(cmd);
                        showPassiveNotification(i18n("Launch command copied to clipboard"), 2000);
                    } else {
                        showPassiveNotification(i18n("RetroArch not found"), 3000);
                    }
                }
            }
        }

        onLaunched: {
            romGrid.launchOrDownload(cardFrame.index);
        }
        onContextMenuRequested: {
            romContextMenu.popup();
        }
    }

    Kirigami.PromptDialog {
        id: downloadProgressDialog
        property var pendingRom: null
        title: pendingRom ? i18n("Downloading %1", pendingRom.fileName) : i18n("Downloading…")
        standardButtons: Kirigami.Dialog.Cancel
        onRejected: pendingRom = null

        ColumnLayout {
            QQC2.ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: rommFileDownloader.progress
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: rommFileDownloader.statusText
                opacity: 0.7
                font.italic: true
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Kirigami.PromptDialog {
        id: filePickerDialog
        property var pendingRom: null
        title: i18n("Select file to download")
        standardButtons: Kirigami.Dialog.Cancel

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing
            Repeater {
                model: filePickerDialog.pendingRom ? filePickerDialog.pendingRom.fileNames : []
                QQC2.Button {
                    required property string modelData
                    text: modelData
                    Layout.fillWidth: true
                    onClicked: {
                        var rom = filePickerDialog.pendingRom;
                        filePickerDialog.close();
                        romGrid.startDownload(rom, modelData);
                    }
                }
            }
        }
    }
}
