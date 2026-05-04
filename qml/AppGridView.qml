import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtCore
import org.kde.kirigami as Kirigami

GridView {
    id: gridView
    model: appModel
    property real scaleFactor: 1.0
    property bool lightsOut: false
    property string viewType: "icon"
    property bool showNames: true

    Settings {
        id: viewSettings
        category: "GridView"
        property alias scaleFactor: gridView.scaleFactor
        property alias viewType: gridView.viewType
        property alias showNames: gridView.showNames
    }

    cellWidth: viewType === "hero" ? 300 * scaleFactor : viewType === "grid" ? 155 * scaleFactor : 140 * scaleFactor
    cellHeight: {
        if (viewType === "hero")
            return (showNames ? 140 : 116) * scaleFactor;
        if (viewType === "grid")
            return (showNames ? 250 : 232) * scaleFactor;
        return (showNames ? 160 : 120) * scaleFactor;
    }

    clip: true
    focus: true
    keyNavigationEnabled: true
    property bool active: true

    onActiveFocusChanged: {
        if (!activeFocus)
            currentIndex = -1;
    }

    Connections {
        target: launcher
        function onRomCoreMissing(platformSlug, rom) {
            if (gridView.active) {
                mainCorePicker.platformSlug = platformSlug;
                mainCorePicker.pendingRom = rom;
                mainCorePicker.appIndex = -1;
                mainCorePicker.open();
            }
        }
    }

    TapHandler {
        onTapped: {
            gridView.currentIndex = -1;
        }
    }

    Shortcut {
        sequence: "Return"
        enabled: gridView.active && gridView.currentIndex >= 0
        onActivated: {
            var app = appModel.getApp(gridView.currentIndex);
            launcher.launchEntry(app);
        }
    }
    Shortcut {
        sequence: "Delete"
        enabled: gridView.active && gridView.currentIndex >= 0
        onActivated: {
            var app = appModel.getApp(gridView.currentIndex);
            confirmDeleteAppDialog.runtimeType = app.runtimeType;
            confirmDeleteAppDialog.payload = gridView.currentIndex;
            confirmDeleteAppDialog.open();
        }
    }
    Shortcut {
        sequence: "Shift+Delete"
        enabled: gridView.active && gridView.currentIndex >= 0 && appModel.getApp(gridView.currentIndex).runtimeType !== "native"
        onActivated: {
            confirmDeleteDialog.payload = gridView.currentIndex;
            confirmDeleteDialog.open();
        }
    }

    delegate: Item {
        id: delegateRoot
        width: gridView.cellWidth
        height: gridView.cellHeight

        required property int index
        required property string appId
        required property string name
        required property string exePath
        required property string runtimeType
        required property string protonPath
        required property string protonPrefix
        required property string wineBinary
        required property string winePrefix
        required property string iconPath
        required property string gridPath
        required property string heroPath
        required property string logoPath
        required property string launchOptions
        required property bool enableLogging
        required property int steamAppId
        required property string platformSlug
        required property string customCorePath

        property bool isSelected: gridView.currentIndex === delegateRoot.index
        readonly property bool hasPrefix: runtimeType === "proton" || runtimeType === "wine"

        Rectangle {
            id: cardBg
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing
            radius: Kirigami.Units.cornerRadius
            color: "transparent"
            layer.enabled: gridView.viewType !== "icon"
            scale: delegateRoot.isSelected ? 1.06 : 1.0
            z: delegateRoot.isSelected ? 2 : 0
            Behavior on scale {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }

            // ── Steam badge ─────────────────────────────────────────
            Kirigami.Icon {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: Kirigami.Units.smallSpacing
                width: 14 * gridView.scaleFactor
                height: 14 * gridView.scaleFactor
                source: "steam"
                visible: delegateRoot.runtimeType === "steam"
                z: 10
            }

            // ── Retroarch badge ─────────────────────────────────────
            Kirigami.Icon {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: Kirigami.Units.smallSpacing
                width: 14 * gridView.scaleFactor
                height: 14 * gridView.scaleFactor
                source: "input-gaming"
                visible: delegateRoot.runtimeType === "retroarch"
                z: 10
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

            // ── Icon mode ────────────────────────────────────────────────────
            ColumnLayout {
                visible: gridView.viewType === "icon"
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing + 2
                spacing: Kirigami.Units.smallSpacing

                Item {
                    Layout.fillHeight: true
                    visible: !gridView.showNames
                }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 80 * gridView.scaleFactor
                    Layout.preferredHeight: 80 * gridView.scaleFactor
                    radius: Kirigami.Units.cornerRadius
                    color: "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 70 * gridView.scaleFactor
                        height: 70 * gridView.scaleFactor
                        source: {
                            if (delegateRoot.iconPath === "")
                                return "";
                            if (delegateRoot.iconPath.startsWith("/"))
                                return "file://" + delegateRoot.iconPath;
                            return "image://icon/" + delegateRoot.iconPath;
                        }
                        visible: delegateRoot.iconPath !== ""
                        fillMode: Image.PreserveAspectFit
                        sourceSize: Qt.size(128, 128)
                    }

                    QQC2.Label {
                        anchors.centerIn: parent
                        text: delegateRoot.name.charAt(0).toUpperCase()
                        font.pixelSize: 32 * gridView.scaleFactor
                        font.bold: true
                        color: Kirigami.Theme.highlightColor
                        visible: delegateRoot.iconPath === ""
                    }
                }

                Item {
                    Layout.fillHeight: true
                    visible: !gridView.showNames
                }

                QQC2.Label {
                    text: delegateRoot.name
                    visible: gridView.showNames
                    color: gridView.lightsOut ? "#ffffff" : Kirigami.Theme.textColor
                    font.pixelSize: 12 * gridView.scaleFactor
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

            // ── Grid / Hero mode ─────────────────────────────────────────────
            Item {
                visible: gridView.viewType !== "icon"
                anchors.fill: parent

                // Background art image
                Image {
                    id: artImage
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    source: {
                        var p = gridView.viewType === "hero" ? delegateRoot.heroPath : delegateRoot.gridPath;
                        return p !== "" ? "file://" + p : "";
                    }
                    visible: source !== ""
                }

                // Fallback placeholder when no art
                Rectangle {
                    anchors.fill: parent
                    visible: artImage.source === ""
                    color: Kirigami.Theme.alternateBackgroundColor

                    Image {
                        anchors.centerIn: parent
                        width: 48 * gridView.scaleFactor
                        height: 48 * gridView.scaleFactor
                        source: {
                            if (delegateRoot.iconPath === "")
                                return "";
                            if (delegateRoot.iconPath.startsWith("/"))
                                return "file://" + delegateRoot.iconPath;
                            return "image://icon/" + delegateRoot.iconPath;
                        }
                        visible: delegateRoot.iconPath !== ""
                        fillMode: Image.PreserveAspectFit
                        sourceSize: Qt.size(96, 96)
                    }
                    QQC2.Label {
                        anchors.centerIn: parent
                        visible: delegateRoot.iconPath === ""
                        text: delegateRoot.name.charAt(0).toUpperCase()
                        font.pixelSize: 28 * gridView.scaleFactor
                        font.bold: true
                        color: Kirigami.Theme.highlightColor
                    }
                }

                // Logo overlay — hero mode only, when hero art is present
                Image {
                    visible: gridView.viewType === "hero" && delegateRoot.logoPath !== "" && artImage.source !== ""
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: artNameOverlay.visible ? artNameOverlay.top : parent.bottom
                    anchors.margins: Kirigami.Units.smallSpacing * 2
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    source: delegateRoot.logoPath !== "" ? "file://" + delegateRoot.logoPath : ""
                }

                // Name overlay at bottom
                Rectangle {
                    id: artNameOverlay
                    visible: gridView.showNames
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
                        font.pixelSize: 11 * gridView.scaleFactor
                        font.bold: true
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                    }
                }
            }

            // ── Selection / hover border (above art in all modes) ────────────
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

            // ── Launch flash ─────────────────────────────────────────────────
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

                onDoubleClicked: function (mouse) {
                    if (mouse.button === Qt.LeftButton && !Qt.styleHints.singleClickActivation) {
                        launchAnim.start();
                        flashAnim.start();
                        var app = appModel.getApp(delegateRoot.index);
                        launcher.launchEntry(app);
                    }
                }

                onClicked: function (mouse) {
                    gridView.currentIndex = delegateRoot.index;
                    gridView.forceActiveFocus();
                    if (mouse.button === Qt.LeftButton && Qt.styleHints.singleClickActivation) {
                        launchAnim.start();
                        flashAnim.start();
                        var app = appModel.getApp(delegateRoot.index);
                        launcher.launchEntry(app);
                    } else if (mouse.button === Qt.RightButton) {
                        contextMenu.popup();
                    }
                }
            }
        }

        QQC2.Menu {
            id: contextMenu
            QQC2.MenuItem {
                property bool isRunning: launcher.runningExePaths.indexOf(delegateRoot.exePath) >= 0
                text: isRunning ? i18n("Stop") : i18n("Launch")
                icon.name: isRunning ? "media-playback-stop" : "media-playback-start"
                onTriggered: {
                    var app = appModel.getApp(delegateRoot.index);
                    if (isRunning) {
                        launcher.stopEntry(app);
                    } else {
                        launchAnim.start();
                        flashAnim.start();
                        launcher.launchEntry(app);
                    }
                }
            }
            QQC2.MenuItem {
                visible: delegateRoot.runtimeType === "steam" && delegateRoot.steamAppId > 0
                text: i18n("View in Steam")
                icon.name: "steam"
                onTriggered: Qt.openUrlExternally("steam://nav/games/details/" + delegateRoot.steamAppId)
            }
            QQC2.MenuItem {
                visible: delegateRoot.runtimeType === "retroarch"
                text: i18n("Change Core…")
                icon.name: "media-record"
                onTriggered: {
                    mainCorePicker.platformSlug = delegateRoot.platformSlug;
                    mainCorePicker.pendingRom = null;
                    mainCorePicker.appIndex = delegateRoot.index;
                    mainCorePicker.launchAfterPick = false;
                    mainCorePicker.open();
                }
            }
            QQC2.MenuItem {
                visible: delegateRoot.runtimeType === "retroarch"
                text: i18n("Copy Launch Command")
                icon.name: "edit-copy"
                onTriggered: {
                    var app = appModel.getApp(delegateRoot.index);
                    var rom = {
                        "localRomPath": app.exePath,
                        "platformSlug": app.platformSlug,
                        "name": app.name,
                        "romId": 0,
                        "customCorePath": app.customCorePath
                    };
                    var cmd = launcher.buildRomLaunchCommand(rom);
                    if (cmd !== "")
                        launcher.copyToClipboard(cmd);
                }
            }
            QQC2.MenuItem {
                visible: delegateRoot.runtimeType !== "steam"
                text: i18n("Launch with logging")
                icon.name: "text-x-log"
                onTriggered: {
                    launchAnim.start();
                    flashAnim.start();
                    var app = appModel.getApp(delegateRoot.index);
                    app.enableLogging = true;
                    launcher.launchEntry(app);
                    Qt.openUrlExternally("file://" + launcher.logDir());
                }
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                visible: delegateRoot.hasPrefix
                text: i18n("Run another EXE in this prefix")
                icon.name: "system-run"
                onTriggered: {
                    runExeDialog.appIndex = delegateRoot.index;
                    runExeDialog.open();
                }
            }
            QQC2.MenuSeparator {
                visible: delegateRoot.hasPrefix
            }
            QQC2.MenuItem {
                text: i18n("Create start menu entry")
                icon.name: "application-menu"
                onTriggered: {
                    var app = appModel.getApp(delegateRoot.index);
                    desktopWriter.createStartMenuEntry(app);
                }
            }
            QQC2.MenuItem {
                text: i18n("Create desktop shortcut")
                icon.name: "user-desktop"
                onTriggered: {
                    var app = appModel.getApp(delegateRoot.index);
                    desktopWriter.createDesktopShortcut(app);
                }
            }
            QQC2.Menu {
                enabled: delegateRoot.hasPrefix
                title: i18n("&Wine Utilities")
                icon.name: "wine"

                QQC2.MenuItem {
                    text: i18n("Run Winecfg")
                    icon.name: "preferences-system"
                    onTriggered: {
                        var app = appModel.getApp(delegateRoot.index);
                        launcher.runWinecfg(app);
                    }
                }
                QQC2.MenuItem {
                    text: i18n("Run Regedit")
                    icon.name: "document-edit"
                    onTriggered: {
                        var app = appModel.getApp(delegateRoot.index);
                        launcher.runRegedit(app);
                    }
                }
                QQC2.MenuItem {
                    text: i18n("Run Winetricks")
                    icon.name: "tools"
                    onTriggered: {
                        if (!launcher.isWinetricksAvailable()) {
                            winetricksNotFoundDialog.open();
                            return;
                        }
                        var app = appModel.getApp(delegateRoot.index);
                        launcher.runWinetricks(app);
                    }
                }
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                text: i18n("Open log folder")
                icon.name: "folder-open"
                onTriggered: Qt.openUrlExternally("file://" + launcher.logDir())
            }
            QQC2.MenuItem {
                visible: delegateRoot.hasPrefix
                text: i18n("Open prefix folder")
                icon.name: "folder-open"
                onTriggered: {
                    var prefix = delegateRoot.runtimeType === "proton" ? delegateRoot.protonPrefix : delegateRoot.winePrefix;
                    if (prefix !== "")
                        Qt.openUrlExternally("file://" + prefix);
                }
                enabled: (delegateRoot.runtimeType === "proton" ? delegateRoot.protonPrefix : delegateRoot.winePrefix) !== ""
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                text: i18n("Edit")
                icon.name: "document-edit"
                onTriggered: addDialog.openForEdit(delegateRoot.index)
            }
            QQC2.MenuItem {
                text: i18n("Remove")
                icon.name: "edit-delete"
                onTriggered: {
                    confirmDeleteAppDialog.runtimeType = delegateRoot.runtimeType;
                    confirmDeleteAppDialog.payload = delegateRoot.index;
                    confirmDeleteAppDialog.open();
                }
            }
            QQC2.MenuItem {
                visible: delegateRoot.hasPrefix
                text: i18n("Remove and Delete Prefix")
                icon.name: "edit-delete"
                onTriggered: {
                    confirmDeleteDialog.payload = delegateRoot.index;
                    confirmDeleteDialog.open();
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: confirmDeleteDialog
        property var payload
        title: i18n("Delete both app and prefix?")
        subtitle: i18n("This will delete both the app and the prefix?")
        onAccepted: appModel.removeAndCleanApp(payload)
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    }

    Kirigami.PromptDialog {
        id: confirmDeleteAppDialog
        property var payload
        property string runtimeType: ""
        title: i18n("Delete the app?")
        subtitle: runtimeType === "native" || runtimeType === "retroarch" || runtimeType === "steam" ? i18n("This will delete the app from the library.") : i18n("This will delete the app but preserve the prefix folder.")
        onAccepted: appModel.removeApp(payload)
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    }

    Kirigami.PromptDialog {
        id: winetricksNotFoundDialog
        title: i18n("Winetricks not found")
        subtitle: i18n("Winetricks is not installed on your system. Please install it using your package manager.")
        standardButtons: Kirigami.Dialog.Ok
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: gridView.count === 0
        text: i18n("No apps or games added yet")
        explanation: i18n("Click \"Add App/Game\" to get started")
        icon.name: "games-config-custom"
    }
}
