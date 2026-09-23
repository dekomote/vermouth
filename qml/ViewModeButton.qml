import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

RowLayout {
    id: root
    spacing: 0

    property string iconName: ""
    property string tooltipText: i18n("Switch view type")

    signal mainClicked
    signal menuRequested(var anchorButton)

    QQC2.ToolButton {
        id: mainBtn
        focusPolicy: Qt.NoFocus
        icon.name: root.iconName
        icon.color: Kirigami.Theme.textColor
        onClicked: root.mainClicked()
        QQC2.ToolTip.text: root.tooltipText
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    QQC2.ToolButton {
        focusPolicy: Qt.NoFocus
        icon.name: "go-down-symbolic"
        icon.width: Kirigami.Units.iconSizes.small
        icon.height: Kirigami.Units.iconSizes.small
        icon.color: Kirigami.Theme.textColor
        onClicked: root.menuRequested(mainBtn)
        QQC2.ToolTip.text: root.tooltipText
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }
}
