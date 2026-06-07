import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root
    visible: isFlatpak
    spacing: Kirigami.Units.smallSpacing

    readonly property string command: "flatpak override --user --talk-name=org.freedesktop.Flatpak com.dekomote.vermouth"

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: true
        type: Kirigami.MessageType.Information
        text: i18n("Optional: HDR and RetroArch support need host access when running as a Flatpak. If you'd like to use them, you can grant Vermouth permission to talk to the host and restart it - otherwise feel free to ignore this. To enable, run:")
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing

        QQC2.TextField {
            id: cmdField
            Layout.fillWidth: true
            readOnly: true
            font.family: "monospace"
            text: root.command
        }
        QQC2.ToolButton {
            icon.name: "edit-copy"
            QQC2.ToolTip.text: i18n("Copy command")
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: {
                cmdField.selectAll();
                cmdField.copy();
                cmdField.deselect();
            }
        }
    }
}
