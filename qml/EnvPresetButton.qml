import QtQuick
import QtQuick.Controls as QQC2

// Opens the preset table; the chosen preset's variables are added to `targetModel`.
QQC2.Button {
    id: root

    property var targetModel

    text: i18n("Add Preset")
    icon.name: "list-add-symbolic"
    onClicked: presetDialog.open()

    EnvPresetDialog {
        id: presetDialog
        targetModel: root.targetModel
    }
}
