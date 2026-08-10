import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "TimeUtils.js" as TimeUtils

Item {
    id: frameRoot

    required property int index
    required property var gv

    property string badgeType: ""
    property string displayName: ""
    property string artSource: ""
    property string iconFallback: ""
    property string heroLogo: ""
    property string platformLogo: ""

    signal launched
    signal contextMenuRequested

    function playLaunchAnimation() {
        launchAnim.start();
        flashAnim.start();
    }

    readonly property bool isSelected: gv && gv.currentIndex === frameRoot.index
    property bool hovered: false

    width: gv ? gv.cellWidth : 140
    height: gv ? gv.cellHeight : 120

    Rectangle {
        id: cardBg
        anchors.fill: parent
        anchors.margins: Kirigami.Units.mediumSpacing
        radius: Kirigami.Units.cornerRadius
        color: "transparent"
        scale: frameRoot.isSelected ? 1.03 : frameRoot.hovered ? 1.02 : 1.0
        z: frameRoot.isSelected ? 2 : 0

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

        Kirigami.Icon {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: Kirigami.Units.smallSpacing
            width: 28 * frameRoot.gv.scaleFactor
            height: 28 * frameRoot.gv.scaleFactor
            source: frameRoot.badgeType === "steam" ? "steam" : frameRoot.badgeType === "retroarch" ? "input-gaming" : ""
            visible: frameRoot.badgeType !== ""
            z: 10
        }

        ColumnLayout {
            visible: frameRoot.gv && frameRoot.gv.viewType === "icon"
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing + 2
            spacing: Kirigami.Units.smallSpacing

            Item {
                Layout.fillHeight: true
                visible: frameRoot.gv && !frameRoot.gv.showNames
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 80 * frameRoot.gv.scaleFactor
                Layout.preferredHeight: 80 * frameRoot.gv.scaleFactor
                radius: Kirigami.Units.cornerRadius
                color: "transparent"

                Image {
                    anchors.centerIn: parent
                    width: 70 * frameRoot.gv.scaleFactor
                    height: 70 * frameRoot.gv.scaleFactor
                    source: frameRoot.artSource
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    mipmap: true
                    visible: frameRoot.artSource !== ""
                    sourceSize: Qt.size(128, 128)
                }

                QQC2.Label {
                    anchors.centerIn: parent
                    text: frameRoot.displayName.charAt(0).toUpperCase()
                    font.pixelSize: 32 * frameRoot.gv.scaleFactor
                    font.bold: true
                    color: Kirigami.Theme.highlightColor
                    visible: frameRoot.artSource === ""
                }

                Rectangle {
                    visible: frameRoot.platformLogo !== ""
                    anchors.top: parent.top
                    anchors.right: parent.right
                    width: 18 * frameRoot.gv.scaleFactor
                    height: 18 * frameRoot.gv.scaleFactor
                    radius: 3
                    color: Qt.rgba(0, 0, 0, 0.55)
                    Image {
                        anchors.centerIn: parent
                        width: parent.width - 2
                        height: parent.height - 2
                        source: frameRoot.platformLogo
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        mipmap: true
                        sourceSize: Qt.size(32, 32)
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                visible: frameRoot.gv && !frameRoot.gv.showNames
            }

            QQC2.Label {
                text: frameRoot.displayName
                visible: frameRoot.gv && frameRoot.gv.showNames
                color: frameRoot.gv && frameRoot.gv.lightsOut ? "#ffffff" : Kirigami.Theme.textColor
                font.pixelSize: 12 * frameRoot.gv.scaleFactor
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

        Item {
            visible: frameRoot.gv && frameRoot.gv.viewType !== "icon"
            anchors.fill: parent

            Image {
                id: artImage
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                mipmap: true
                source: frameRoot.artSource
                visible: source !== ""
            }

            Rectangle {
                anchors.fill: parent
                visible: frameRoot.artSource == ""

                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: Qt.darker(Kirigami.Theme.alternateBackgroundColor, 1.1)
                    }
                    GradientStop {
                        position: 1.0
                        color: Kirigami.Theme.alternateBackgroundColor
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.smallSpacing
                    width: parent.width - Kirigami.Units.largeSpacing * 2

                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 48 * frameRoot.gv.scaleFactor
                        Layout.preferredHeight: 48 * frameRoot.gv.scaleFactor
                        source: frameRoot.iconFallback
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        mipmap: true
                        visible: frameRoot.iconFallback !== ""
                        sourceSize: Qt.size(96, 96)
                    }
                    QQC2.Label {
                        Layout.alignment: Qt.AlignHCenter
                        visible: frameRoot.iconFallback === ""
                        text: frameRoot.displayName.charAt(0).toUpperCase()
                        font.pixelSize: 36 * frameRoot.gv.scaleFactor
                        font.bold: true
                        color: Kirigami.Theme.highlightColor
                        opacity: 0.4
                    }
                }
            }

            Image {
                visible: frameRoot.gv && frameRoot.gv.viewType === "hero" && frameRoot.heroLogo !== "" && artImage.source !== ""
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: artNameOverlay.visible ? artNameOverlay.top : parent.bottom
                anchors.margins: Kirigami.Units.smallSpacing * 2
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                mipmap: true
                source: frameRoot.heroLogo
            }

            Rectangle {
                visible: frameRoot.platformLogo !== ""
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: Kirigami.Units.smallSpacing
                width: 28 * frameRoot.gv.scaleFactor
                height: 28 * frameRoot.gv.scaleFactor
                radius: 4
                color: Qt.rgba(0, 0, 0, 0.55)
                Image {
                    anchors.centerIn: parent
                    width: parent.width - 4
                    height: parent.height - 4
                    source: frameRoot.platformLogo
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    mipmap: true
                    sourceSize: Qt.size(48, 48)
                }
            }

            Rectangle {
                id: artNameOverlay
                visible: frameRoot.gv && frameRoot.gv.showNames
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
                    text: frameRoot.displayName
                    color: "#ffffff"
                    font.pixelSize: 11 * frameRoot.gv.scaleFactor
                    font.bold: true
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                }
            }

            Rectangle {
                id: playTimeChip
                visible: frameRoot.gv && frameRoot.gv.viewType !== "icon" && frameRoot.gv.showPlayTime && frameRoot.playTime > 0
                anchors.bottom: artNameOverlay.visible ? artNameOverlay.top : parent.bottom
                anchors.right: parent.right
                anchors.bottomMargin: Kirigami.Units.smallSpacing
                anchors.rightMargin: Kirigami.Units.smallSpacing
                height: playTimeLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
                width: playTimeRow.implicitWidth + Kirigami.Units.smallSpacing * 2
                radius: Kirigami.Units.cornerRadius
                color: Qt.rgba(0, 0, 0, 0.65)

                Row {
                    id: playTimeRow
                    anchors.centerIn: parent
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 12 * frameRoot.gv.scaleFactor
                        height: 12 * frameRoot.gv.scaleFactor
                        source: "clock-symbolic"
                        color: "#ffffff"
                    }
                    QQC2.Label {
                        id: playTimeLabel
                        text: TimeUtils.formatPlayTime(frameRoot.playTime)
                        color: "#ffffff"
                        font.pixelSize: 10 * frameRoot.gv.scaleFactor
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: 1
            color: "transparent"
            border.color: frameRoot.isSelected ? Kirigami.Theme.highlightColor : mouseArea.containsMouse ? Qt.darker(Kirigami.Theme.highlightColor, 1.5) : "transparent"
            border.width: frameRoot.isSelected ? 3 : mouseArea.containsMouse ? 1 : 0
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

        Rectangle {
            id: launchFlash
            anchors.fill: parent
            radius: 1
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

            onContainsMouseChanged: frameRoot.hovered = containsMouse
            onClicked: function (mouse) {
                if (frameRoot.gv) {
                    frameRoot.gv.currentIndex = frameRoot.index;
                    frameRoot.gv.forceActiveFocus();
                }
                if (mouse.button === Qt.LeftButton && Qt.styleHints.singleClickActivation) {
                    frameRoot.playLaunchAnimation();
                    frameRoot.launched();
                } else if (mouse.button === Qt.RightButton) {
                    frameRoot.contextMenuRequested();
                }
            }
            onDoubleClicked: function (mouse) {
                if (mouse.button === Qt.LeftButton && !Qt.styleHints.singleClickActivation) {
                    frameRoot.playLaunchAnimation();
                    frameRoot.launched();
                }
            }
        }
    }
}
