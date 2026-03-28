import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: i18n("Run EXE in Prefix")
    preferredWidth: Kirigami.Units.gridUnit * 26
    padding: Kirigami.Units.largeSpacing
    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

    property int appIndex: -1

    onAccepted: {
        if (appIndex >= 0 && exeField.text !== "") {
            var app = appModel.getApp(appIndex);
            launcher.runInPrefix(app, exeField.text);
        }
    }

    onOpened: exeField.text = ""

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            text: dialog.appIndex >= 0 ? i18n("Run an executable using the same prefix as \"%1\"", appModel.getApp(dialog.appIndex).name) : ""
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Kirigami.FormLayout {
            Layout.topMargin: Kirigami.Units.largeSpacing
            RowLayout {
                Kirigami.FormData.label: i18n("Executable (.exe):")
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
        title: i18n("Select Executable")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Executables (*.exe)"), i18n("All files (*)")]
        onAccepted: exeField.text = selectedFile.toString().replace("file://", "")
    }
}
