import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

FocusScope {
    id: root

    property bool lightsOut: false
    property string viewType: "grid"
    property real scaleFactor: 1.0
    property bool showNames: true

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

    Connections {
        target: rommModel
        function onPlatformsFetched(p) {
            var logoMap = {};
            for (var i = 0; i < p.length; i++) {
                if (p[i].logoUrl)
                    logoMap[p[i].slug] = p[i].logoUrl;
            }
            root.platformLogoMap = logoMap;
            root.platforms = [
                {
                    id: 0,
                    name: i18n("All")
                }
            ].concat(p);
        }
        function onRomsFetched(more) {
            root.hasMore = more;
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
        onTriggered: rommModel.fetchRoms(root.currentPlatformId, root.searchText)
    }

    // ── ROM grid area ────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ROM GridView
        GridView {
            id: romGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            keyNavigationEnabled: true
            focus: true
            model: rommModel

            onActiveFocusChanged: {
                if (!activeFocus)
                    currentIndex = -1;
            }

            cellWidth: root.viewType === "hero" ? 300 * root.scaleFactor : root.viewType === "grid" ? 155 * root.scaleFactor : 140 * root.scaleFactor
            cellHeight: {
                if (root.viewType === "hero")
                    return (root.showNames ? 140 : 116) * root.scaleFactor;
                if (root.viewType === "grid")
                    return (root.showNames ? 250 : 232) * root.scaleFactor;
                return (root.showNames ? 160 : 120) * root.scaleFactor;
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: !rommModel.busy && rommModel.count === 0 && root.currentPlatformId >= 0
                text: i18n("No ROMs found")
                icon.name: "folder-games"
            }
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: !rommModel.busy && root.currentPlatformId === -1 && root.platforms.length > 0
                text: i18n("Select a platform")
                icon.name: "folder-games"
            }
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: !rommModel.busy && root.platforms.length === 0
                text: settingsManager.rommServerUrl !== "" ? i18n("No platforms") : i18n("Configure RomM\nin Settings")
                icon.name: settingsManager.rommServerUrl !== "" ? "folder-games" : "configure"
            }

            Keys.onReturnPressed: {
                if (romGrid.activeFocus)
                    root.launchOrDownload(currentIndex);
            }

            function checkLoadMore() {
                if (root.hasMore && !rommModel.busy && (contentHeight <= height || contentY + height >= contentHeight - cellHeight * 2)) {
                    var nextPage = Math.floor(rommModel.count / 50) + 1;
                    rommModel.fetchRoms(root.currentPlatformId, root.searchText, nextPage);
                }
            }

            onContentYChanged: checkLoadMore()
            onHeightChanged: checkLoadMore()

            delegate: Item {
                id: delegateRoot
                width: romGrid.cellWidth
                height: romGrid.cellHeight

                required property int index
                required property int romId
                required property string name
                required property string fileName
                required property var fileNames
                required property string platformSlug
                required property string coverUrl
                required property string localCover
                required property double fileSizeBytes

                property bool isSelected: romGrid.currentIndex === delegateRoot.index
                property string artSource: localCover !== "" ? "file://" + localCover : (coverUrl !== "" ? coverUrl : "")
                property string platformLogo: root.platformLogoMap[delegateRoot.platformSlug] ?? ""

                Rectangle {
                    id: cardBg
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing
                    radius: Kirigami.Units.cornerRadius
                    color: "transparent"
                    layer.enabled: root.viewType !== "icon"
                    scale: delegateRoot.isSelected ? 1.06 : 1.0
                    z: delegateRoot.isSelected ? 2 : 0
                    Behavior on scale {
                        NumberAnimation {
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }

                    SequentialAnimation {
                        id: launchAnim
                        NumberAnimation {
                            target: cardBg
                            property: "scale"
                            to: 0.9
                            duration: 100
                            easing.type: Easing.InQuad
                        }
                        NumberAnimation {
                            target: cardBg
                            property: "scale"
                            to: 1.0
                            duration: 200
                            easing.type: Easing.OutBack
                        }
                    }

                    // ── Icon mode ────────────────────────────────────────────
                    ColumnLayout {
                        visible: root.viewType === "icon"
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.smallSpacing + 2
                        spacing: Kirigami.Units.smallSpacing

                        Item {
                            Layout.fillHeight: true
                            visible: !root.showNames
                        }

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 80 * root.scaleFactor
                            Layout.preferredHeight: 80 * root.scaleFactor
                            radius: Kirigami.Units.cornerRadius
                            color: "transparent"

                            Image {
                                anchors.centerIn: parent
                                width: 70 * root.scaleFactor
                                height: 70 * root.scaleFactor
                                source: delegateRoot.artSource
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                visible: delegateRoot.artSource !== ""
                                sourceSize: Qt.size(128, 128)
                            }
                            QQC2.Label {
                                anchors.centerIn: parent
                                text: delegateRoot.name.charAt(0).toUpperCase()
                                font.pixelSize: 32 * root.scaleFactor
                                font.bold: true
                                color: Kirigami.Theme.highlightColor
                                visible: delegateRoot.artSource === ""
                            }
                            Rectangle {
                                visible: delegateRoot.platformLogo !== ""
                                anchors.top: parent.top
                                anchors.right: parent.right
                                width: 18 * root.scaleFactor
                                height: 18 * root.scaleFactor
                                radius: 3
                                color: Qt.rgba(0, 0, 0, 0.55)
                                Image {
                                    anchors.centerIn: parent
                                    width: parent.width - 2
                                    height: parent.height - 2
                                    source: delegateRoot.platformLogo
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                    sourceSize: Qt.size(32, 32)
                                }
                            }
                        }

                        Item {
                            Layout.fillHeight: true
                            visible: !root.showNames
                        }

                        QQC2.Label {
                            text: delegateRoot.name
                            visible: root.showNames
                            color: root.lightsOut ? "#ffffff" : Kirigami.Theme.textColor
                            font.pixelSize: 12 * root.scaleFactor
                            font.bold: true
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                        }
                    }

                    // ── Grid / Hero mode ─────────────────────────────────────
                    Item {
                        visible: root.viewType !== "icon"
                        anchors.fill: parent

                        Image {
                            id: artImage
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            source: delegateRoot.artSource
                            visible: source !== ""
                        }

                        Rectangle {
                            anchors.fill: parent
                            visible: artImage.source === ""
                            color: Kirigami.Theme.alternateBackgroundColor

                            QQC2.Label {
                                anchors.centerIn: parent
                                text: delegateRoot.name.charAt(0).toUpperCase()
                                font.pixelSize: 28 * root.scaleFactor
                                font.bold: true
                                color: Kirigami.Theme.highlightColor
                            }
                        }

                        // ── Platform logo badge ──────────────────────────────
                        Rectangle {
                            visible: delegateRoot.platformLogo !== ""
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: Kirigami.Units.smallSpacing
                            width: 28 * root.scaleFactor
                            height: 28 * root.scaleFactor
                            radius: 4
                            color: Qt.rgba(0, 0, 0, 0.55)

                            Image {
                                anchors.centerIn: parent
                                width: parent.width - 4
                                height: parent.height - 4
                                source: delegateRoot.platformLogo
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                sourceSize: Qt.size(48, 48)
                            }
                        }

                        Rectangle {
                            id: artNameOverlay
                            visible: root.showNames
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: artNameLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
                            color: Qt.rgba(0, 0, 0, 0.65)

                            QQC2.Label {
                                id: artNameLabel
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: Kirigami.Units.smallSpacing
                                text: delegateRoot.name
                                color: "#ffffff"
                                font.pixelSize: 11 * root.scaleFactor
                                font.bold: true
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                            }
                        }
                    }

                    // ── Selection / hover border ─────────────────────────────
                    Rectangle {
                        anchors.fill: parent
                        radius: Kirigami.Units.cornerRadius
                        color: "transparent"
                        border.color: delegateRoot.isSelected ? Kirigami.Theme.highlightColor : mouseArea.containsMouse ? Qt.darker(Kirigami.Theme.highlightColor, 1.5) : "transparent"
                        border.width: delegateRoot.isSelected ? 3 : mouseArea.containsMouse ? 1 : 0
                        z: 5
                        Behavior on border.color {
                            ColorAnimation {
                                duration: 150
                                easing.type: Easing.OutCubic
                            }
                        }
                        Behavior on border.width {
                            NumberAnimation {
                                duration: 150
                                easing.type: Easing.OutCubic
                            }
                        }
                    }

                    // ── Launch flash ─────────────────────────────────────────
                    Rectangle {
                        id: launchFlash
                        anchors.fill: parent
                        radius: Kirigami.Units.cornerRadius
                        color: Kirigami.Theme.highlightColor
                        opacity: 0
                        z: 10
                        SequentialAnimation {
                            id: flashAnim
                            NumberAnimation {
                                target: launchFlash
                                property: "opacity"
                                to: 0.3
                                duration: 80
                            }
                            NumberAnimation {
                                target: launchFlash
                                property: "opacity"
                                to: 0
                                duration: 300
                                easing.type: Easing.OutCubic
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        z: 20

                        onClicked: function (mouse) {
                            romGrid.currentIndex = delegateRoot.index;
                            romGrid.forceActiveFocus();
                            if (mouse.button === Qt.LeftButton && Qt.styleHints.singleClickActivation) {
                                launchAnim.start();
                                flashAnim.start();
                                root.launchOrDownload(delegateRoot.index);
                            } else if (mouse.button === Qt.RightButton) {
                                romContextMenu.popup();
                            }
                        }
                        onDoubleClicked: function (mouse) {
                            if (mouse.button === Qt.LeftButton && !Qt.styleHints.singleClickActivation) {
                                launchAnim.start();
                                flashAnim.start();
                                root.launchOrDownload(delegateRoot.index);
                            }
                        }
                    }
                }

                QQC2.Menu {
                    id: romContextMenu
                    QQC2.MenuItem {
                        text: i18n("Launch")
                        icon.name: "media-playback-start"
                        onTriggered: {
                            launchAnim.start();
                            flashAnim.start();
                            root.launchOrDownload(delegateRoot.index);
                        }
                    }
                    QQC2.MenuItem {
                        text: i18n("Launch with Log")
                        icon.name: "text-x-log"
                        onTriggered: {
                            launchAnim.start();
                            flashAnim.start();
                            root.launchOrDownload(delegateRoot.index, true);
                        }
                    }
                    QQC2.MenuItem {
                        readonly property string _firstCached: rommFileDownloader.cachedRomPath(delegateRoot.romId, delegateRoot.fileName)
                        text: _firstCached !== "" ? i18n("ROM cached locally") : i18n("Download ROM (%1 MB)").arg((delegateRoot.fileSizeBytes / (1024 * 1024)).toFixed(1))
                        icon.name: "download"
                        enabled: _firstCached === "" && !rommFileDownloader.busy
                        onTriggered: {
                            var rom = rommModel.getRom(delegateRoot.index);
                            var files = rom.fileNames;
                            if (!files || files.length <= 1)
                                root.startDownload(rom, files ? files[0] : rom.fileName);
                            else
                                root.openFilePicker(rom);
                        }
                    }
                    QQC2.MenuItem {
                        text: i18n("Change Core…")
                        icon.name: "media-record"
                        onTriggered: {
                            var rom = rommModel.getRom(delegateRoot.index);
                            mainCorePicker.platformSlug = delegateRoot.platformSlug;
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
                            var rom = rommModel.getRom(delegateRoot.index);
                            var cached = rommFileDownloader.cachedRomPath(delegateRoot.romId, delegateRoot.fileName);
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
            }
        }
    }

    // ── Download progress dialog ─────────────────────────────────────────────
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

    // ── Multi-file picker dialog ─────────────────────────────────────────────
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
                        root.startDownload(rom, modelData);
                    }
                }
            }
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
}
