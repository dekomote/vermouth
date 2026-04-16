import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtCore
import org.kde.kirigami as Kirigami

GridView {
    id: gridView
    model: appModel
    property real scaleFactor: 1.0

    Settings {
        id: viewSettings
        category: "GridView"
        property alias scaleFactor: gridView.scaleFactor
    }
    cellWidth: 140 * scaleFactor
    cellHeight: 170 * scaleFactor
    clip: true
    focus: true
    keyNavigationEnabled: true

    property int selectedIndex: currentIndex

    onCurrentIndexChanged: selectedIndex = currentIndex

    TapHandler {
        onTapped: {
            gridView.currentIndex = -1;
            gridView.selectedIndex = -1;
        }
    }

    Shortcut {
        sequence: "Return"
        enabled: gridView.selectedIndex >= 0
        onActivated: {
            var app = appModel.getApp(gridView.selectedIndex);
            launcher.launchEntry(app);
        }
    }
    Shortcut {
        sequence: "Delete"
        enabled: gridView.selectedIndex >= 0
        onActivated: {
            confirmDeleteAppDialog.payload = gridView.selectedIndex;
            confirmDeleteAppDialog.open();
        }
    }
    Shortcut {
        sequence: "Shift+Delete"
        enabled: gridView.selectedIndex >= 0
        onActivated: {
            confirmDeleteDialog.payload = gridView.selectedIndex;
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
        required property string launchOptions
        required property bool enableLogging

        property bool isSelected: gridView.currentIndex === delegateRoot.index

        Rectangle {
            id: cardBg
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing
            radius: Kirigami.Units.cornerRadius
            color: Kirigami.Theme.backgroundColor
            border.color: delegateRoot.isSelected ? Kirigami.Theme.highlightColor : mouseArea.containsMouse ? Qt.darker(Kirigami.Theme.highlightColor, 1.5) : "transparent"
            border.width: delegateRoot.isSelected ? 2 : mouseArea.containsMouse ? 1 : 0

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

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing + 2
                spacing: Kirigami.Units.smallSpacing

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                    Layout.preferredWidth: 80 * gridView.scaleFactor
                    Layout.preferredHeight: 80 * gridView.scaleFactor
                    radius: Kirigami.Units.cornerRadius
                    color: Kirigami.Theme.alternateBackgroundColor

                    Image {
                        anchors.centerIn: parent
                        width: 64 * gridView.scaleFactor
                        height: 64 * gridView.scaleFactor
                        source: delegateRoot.iconPath !== "" ? "file://" + delegateRoot.iconPath : ""
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

                QQC2.Label {
                    text: delegateRoot.name
                    font.pixelSize: 12 * gridView.scaleFactor
                    font.bold: true
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillWidth: true
                    Layout.maximumHeight: 50 * gridView.scaleFactor
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                }

                QQC2.Label {
                    text: {
                        if (delegateRoot.runtimeType === "proton") {
                            var parts = delegateRoot.protonPath.split("/");
                            return parts[parts.length - 1];
                        }
                        return i18n("Wine");
                    }
                    font.pixelSize: 10 * gridView.scaleFactor
                    color: Kirigami.Theme.disabledTextColor
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignBottom
                    Layout.fillWidth: true
                    maximumLineCount: 1
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                cursorShape: Qt.PointingHandCursor

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
                text: i18n("Launch")
                icon.name: "media-playback-start"
                onTriggered: {
                    launchAnim.start();
                    flashAnim.start();
                    var app = appModel.getApp(delegateRoot.index);
                    launcher.launchEntry(app);
                }
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                text: i18n("Run another EXE in this prefix")
                icon.name: "system-run"
                onTriggered: {
                    runExeDialog.appIndex = delegateRoot.index;
                    runExeDialog.open();
                }
            }
            QQC2.MenuSeparator {}
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
                title: i18n("&Wine Utilities")
                icon.name: "wine" // fallback icon

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
                    confirmDeleteAppDialog.payload = delegateRoot.index;
                    confirmDeleteAppDialog.open();
                }
            }
            QQC2.MenuItem {
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
        title: i18n("Delete the app?")
        subtitle: i18n("This will delete the app but preserve the prefix folder.")
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
