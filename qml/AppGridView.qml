import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

GridView {
    id: gridView
    model: appModel
    cellWidth: 140
    cellHeight: 170
    clip: true

    property int selectedIndex: -1

    MouseArea {
        anchors.fill: parent
        z: -1
        onClicked: gridView.selectedIndex = -1
    }

    delegate: Item {
        id: delegateRoot
        width: gridView.cellWidth
        height: gridView.cellHeight

        required property int index
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

        property bool isSelected: gridView.selectedIndex === delegateRoot.index

        Rectangle {
            id: cardBg
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing
            radius: Kirigami.Units.cornerRadius
            color: Kirigami.Theme.backgroundColor
            border.color: delegateRoot.isSelected ? Kirigami.Theme.highlightColor : mouseArea.containsMouse ? Qt.darker(Kirigami.Theme.highlightColor, 1.5) : "transparent"
            border.width: delegateRoot.isSelected ? 2 : mouseArea.containsMouse ? 1 : 0

            Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
            Behavior on border.width { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

            SequentialAnimation {
                id: launchAnim
                NumberAnimation { target: cardBg; property: "scale"; to: 0.9; duration: 100; easing.type: Easing.InQuad }
                NumberAnimation { target: cardBg; property: "scale"; to: 1.0; duration: 200; easing.type: Easing.OutBack }
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
                    NumberAnimation { target: launchFlash; property: "opacity"; to: 0.3; duration: 80 }
                    NumberAnimation { target: launchFlash; property: "opacity"; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing + 2
                spacing: Kirigami.Units.smallSpacing

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 80
                    radius: Kirigami.Units.cornerRadius
                    color: Kirigami.Theme.alternateBackgroundColor

                    Image {
                        anchors.centerIn: parent
                        width: 64
                        height: 64
                        source: delegateRoot.iconPath !== "" ? "file://" + delegateRoot.iconPath : ""
                        visible: delegateRoot.iconPath !== ""
                        fillMode: Image.PreserveAspectFit
                        sourceSize: Qt.size(128, 128)
                    }

                    QQC2.Label {
                        anchors.centerIn: parent
                        text: delegateRoot.name.charAt(0).toUpperCase()
                        font.pixelSize: 32
                        font.bold: true
                        color: Kirigami.Theme.highlightColor
                        visible: delegateRoot.iconPath === ""
                    }
                }

                QQC2.Label {
                    text: delegateRoot.name
                    font.pixelSize: 12
                    font.bold: true
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    Layout.maximumHeight: 32
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                }

                QQC2.Label {
                    text: {
                        if (delegateRoot.runtimeType === "proton") {
                            var parts = delegateRoot.protonPath.split("/")
                            return parts[parts.length - 1]
                        }
                        return "Wine"
                    }
                    font.pixelSize: 10
                    color: Kirigami.Theme.disabledTextColor
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                cursorShape: Qt.PointingHandCursor

                onDoubleClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        launchAnim.start()
                        flashAnim.start()
                        var app = appModel.getApp(delegateRoot.index)
                        launcher.launchEntry(app)
                    }
                }

                onClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        gridView.selectedIndex = delegateRoot.index
                    } else if (mouse.button === Qt.RightButton) {
                        gridView.selectedIndex = delegateRoot.index
                        contextMenu.popup()
                    }
                }
            }
        }

        QQC2.Menu {
            id: contextMenu
            QQC2.MenuItem {
                text: "Launch"
                icon.name: "media-playback-start"
                onTriggered: {
                    launchAnim.start()
                    flashAnim.start()
                    var app = appModel.getApp(delegateRoot.index)
                    launcher.launchEntry(app)
                }
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                text: "Run another EXE in this prefix"
                icon.name: "system-run"
                onTriggered: {
                    runExeDialog.appIndex = delegateRoot.index
                    runExeDialog.open()
                }
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                text: "Create start menu entry"
                icon.name: "application-menu"
                onTriggered: {
                    var app = appModel.getApp(delegateRoot.index)
                    desktopWriter.createStartMenuEntry(app)
                }
            }
            QQC2.MenuItem {
                text: "Create desktop shortcut"
                icon.name: "user-desktop"
                onTriggered: {
                    var app = appModel.getApp(delegateRoot.index)
                    desktopWriter.createDesktopShortcut(app)
                }
            }
            QQC2.MenuItem {
                text: "Open log folder"
                icon.name: "folder-open"
                onTriggered: Qt.openUrlExternally("file://" + launcher.logDir())
            }
            QQC2.MenuItem {
                text: "Open prefix folder"
                icon.name: "folder-open"
                onTriggered: {
                    var prefix = delegateRoot.runtimeType === "proton" ? delegateRoot.protonPrefix : delegateRoot.winePrefix
                    if (prefix !== "")
                        Qt.openUrlExternally("file://" + prefix)
                }
                enabled: (delegateRoot.runtimeType === "proton" ? delegateRoot.protonPrefix : delegateRoot.winePrefix) !== ""
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                text: "Edit"
                icon.name: "document-edit"
                onTriggered: addDialog.openForEdit(delegateRoot.index)
            }
            QQC2.MenuItem {
                text: "Remove"
                icon.name: "edit-delete"
                onTriggered: {
                    confirmDeleteAppDialog.payload = delegateRoot.index
                    confirmDeleteAppDialog.open()
                }
            }
            QQC2.MenuItem {
                text: "Remove and Delete Prefix"
                icon.name: "edit-delete"
                onTriggered: {
                    confirmDeleteDialog.payload = delegateRoot.index
                    confirmDeleteDialog.open()
                }
            }
        }
    }

    Kirigami.PromptDialog {
        property var payload
        id: confirmDeleteDialog
        title: "Delete both app and prefix?"
        subtitle: "This will delete both the app and the prefix?"
        onAccepted: appModel.removeAndCleanApp(payload)
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    }

    Kirigami.PromptDialog {
        property var payload
        id: confirmDeleteAppDialog
        title: "Delete the app?"
        subtitle: "This will delete the app but preserve the prefix folder."
        onAccepted: appModel.removeApp(payload)
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: gridView.count === 0
        text: "No apps or games added yet"
        explanation: "Click \"Add App/Game\" to get started"
        icon.name: "games-config-custom"
    }
}
