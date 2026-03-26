import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

GridView {
    id: gridView
    model: appModel
    cellWidth: 140
    cellHeight: 170
    clip: true

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

        Rectangle {
            id: card
            anchors.fill: parent
            anchors.margins: 6
            radius: 10
            color: mouseArea.containsMouse ? palette.highlight : palette.button
            opacity: mouseArea.containsMouse ? 0.15 : 1.0

            Rectangle {
                anchors.fill: parent
                radius: 10
                color: "transparent"
                border.color: mouseArea.containsMouse ? palette.highlight : "transparent"
                border.width: 2
            }
        }

        Rectangle {
            id: cardBg
            anchors.fill: parent
            anchors.margins: 6
            radius: 10
            color: palette.button
            scale: 1.0
            transformOrigin: Item.Center

            SequentialAnimation {
                id: launchAnim
                NumberAnimation { target: cardBg; property: "scale"; to: 0.9; duration: 100; easing.type: Easing.InQuad }
                NumberAnimation { target: cardBg; property: "scale"; to: 1.0; duration: 200; easing.type: Easing.OutBack }
            }

            // Brief overlay flash on launch
            Rectangle {
                id: launchFlash
                anchors.fill: parent
                radius: 10
                color: palette.highlight
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
                anchors.margins: 8
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 80
                    radius: 8
                    color: palette.alternateBase

                    Image {
                        anchors.centerIn: parent
                        width: 64
                        height: 64
                        source: delegateRoot.iconPath !== "" ? "file://" + delegateRoot.iconPath : ""
                        visible: delegateRoot.iconPath !== ""
                        fillMode: Image.PreserveAspectFit
                        sourceSize: Qt.size(128, 128)
                    }

                    Label {
                        anchors.centerIn: parent
                        text: delegateRoot.name.charAt(0).toUpperCase()
                        font.pixelSize: 32
                        font.bold: true
                        color: palette.highlight
                        visible: delegateRoot.iconPath === ""
                    }
                }

                Label {
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

                Label {
                    text: {
                        if (delegateRoot.runtimeType === "proton") {
                            var parts = delegateRoot.protonPath.split("/")
                            return parts[parts.length - 1]
                        }
                        return "Wine"
                    }
                    font.pixelSize: 10
                    color: palette.placeholderText
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
                    if (mouse.button === Qt.RightButton) {
                        contextMenu.popup()
                    }
                }
            }
        }

        Menu {
            id: contextMenu
            MenuItem {
                text: "Launch"
                icon.name: "media-playback-start"
                onTriggered: {
                    launchAnim.start()
                    flashAnim.start()
                    var app = appModel.getApp(delegateRoot.index)
                    launcher.launchEntry(app)
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "Run another EXE in this prefix"
                icon.name: "system-run"
                onTriggered: {
                    runExeDialog.appIndex = delegateRoot.index
                    runExeDialog.open()
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "Create start menu entry"
                icon.name: "application-menu"
                onTriggered: {
                    var app = appModel.getApp(delegateRoot.index)
                    desktopWriter.createStartMenuEntry(app)
                }
            }
            MenuItem {
                text: "Create desktop shortcut"
                icon.name: "user-desktop"
                onTriggered: {
                    var app = appModel.getApp(delegateRoot.index)
                    desktopWriter.createDesktopShortcut(app)
                }
            }
            MenuItem {
                text: "Open log folder"
                icon.name: "folder-open"
                onTriggered: Qt.openUrlExternally("file://" + launcher.logDir())
            }
            MenuSeparator {}
            MenuItem {
                text: "Edit"
                icon.name: "document-edit"
                onTriggered: addDialog.openForEdit(delegateRoot.index)
            }
            MenuItem {
                text: "Remove"
                icon.name: "edit-delete"
                onTriggered: confirmDeleteAppDialog.payload = delegateRoot.index, confirmDeleteAppDialog.open()
            }
            MenuItem {
                text: "Remove and Delete Prefix"
                icon.name: "edit-delete"
                onTriggered: confirmDeleteDialog.payload = delegateRoot.index, confirmDeleteDialog.open()
            }
        }
    }

    MessageDialog {
        property var payload
        id: confirmDeleteDialog
        text: "This will delete both the app and the prefix"
        informativeText: "Are you sure you want to delete the app and the prefix?"
        onAccepted: appModel.removeAndCleanApp(payload)
        buttons: MessageDialog.Ok | MessageDialog.Cancel
    }

    MessageDialog {
        property var payload
        id: confirmDeleteAppDialog
        text: "This will only delete the app, the prefix will remain intact"
        informativeText: "Are you sure you want to delete the app and preserve the prefix?"
        onAccepted: appModel.removeApp(payload)
        buttons: MessageDialog.Ok | MessageDialog.Cancel
    }


    Label {
        anchors.centerIn: parent
        text: "No apps or games added yet.\nClick \"Add App/Game\" to get started."
        color: palette.placeholderText
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
        visible: gridView.count === 0
    }
    
}
