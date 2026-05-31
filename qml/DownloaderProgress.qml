import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    property double progress: 0
    property string statusText: ""

    spacing: Kirigami.Units.smallSpacing

    QQC2.ProgressBar {
        Layout.fillWidth: true
        from: 0
        to: 1
        value: parent.progress
        indeterminate: parent.progress <= 0
    }
    QQC2.Label {
        visible: text !== ""
        text: parent.statusText
        font: Kirigami.Theme.smallFont
        opacity: 0.75
    }
}
