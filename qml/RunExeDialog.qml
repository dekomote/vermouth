import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: "Run EXE in Prefix"
    preferredWidth: Kirigami.Units.gridUnit * 26
    padding: Kirigami.Units.largeSpacing

    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

    property int appIndex: -1

    onAccepted: {
        if (appIndex >= 0 && exeField.text !== "") {
            var app = appModel.getApp(appIndex)
            launcher.runInPrefix(app, exeField.text)
        }
    }

    onOpened: exeField.text = ""

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: dialog.appIndex >= 0 ? "Run an executable using the same prefix as \"" + appModel.getApp(dialog.appIndex).name + "\"" : ""
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Kirigami.FormLayout {
            RowLayout {
                Kirigami.FormData.label: "Executable (.exe):"
                QQC2.TextField {
                    id: exeField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/setup.exe"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: fileDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select Executable"
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: ["Executables (*.exe)", "All files (*)"]
        onAccepted: exeField.text = selectedFile.toString().replace("file://", "")
    }
}
