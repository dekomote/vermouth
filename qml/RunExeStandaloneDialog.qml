import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: i18n("Run Standalone")
    preferredWidth: Kirigami.Units.gridUnit * 30
    padding: Kirigami.Units.largeSpacing
    bottomPadding: 30
    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: [
        Kirigami.Action {
            text: i18n("Run")
            icon.name: "media-playback-start"
            onTriggered: {
                if (dialog.validate()) {
                    dialog.doLaunch();
                    dialog.close();
                }
            }
        },
        Kirigami.Action {
            text: i18n("Cancel")
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    function openDialog(exePath) {
        exeField.text = exePath || "";
        launchOptionsField.text = "";
        runtimePicker.reset();
        if (exePath && !/\.exe$/i.test(exePath))
            runtimePicker.setRuntimeType("native");
        dialog.open();
    }

    property string validationError: ""

    function validate() {
        if (exeField.text.trim() === "") {
            validationError = i18n("Executable path is required.");
            return false;
        }
        var rtError = runtimePicker.validate();
        if (rtError !== "") {
            validationError = rtError;
            return false;
        }
        validationError = "";
        return true;
    }

    function doLaunch() {
        var entry = {};
        entry["exePath"] = exeField.text;
        entry["launchOptions"] = launchOptionsField.text;
        entry["enableLogging"] = false;

        if (runtimePicker.runtimeType === "proton") {
            const prefix = protonScanner.prefixBasePath() + "/vermouth_default";
            entry["runtimeType"] = "proton";
            entry["protonPath"] = runtimePicker.protonPath;
            entry["protonPrefix"] = prefix;
        } else if (runtimePicker.runtimeType === "wine") {
            const prefix = protonScanner.winePrefixBasePath() + "/vermouth_default";
            entry["runtimeType"] = "wine";
            entry["wineBinary"] = runtimePicker.wineBinary;
            entry["winePrefix"] = prefix;
        } else {
            entry["runtimeType"] = "native";
        }

        launcher.launchEntry(entry);
    }

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            text: dialog.validationError
            visible: dialog.validationError !== ""
        }

        Kirigami.FormLayout {
            id: topForm
            twinFormLayouts: runtimePicker.formLayout

            RowLayout {
                Kirigami.FormData.label: i18n("Executable:")
                QQC2.TextField {
                    id: exeField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/game"
                }
                QQC2.Button {
                    icon.name: "document-open-symbolic"
                    onClicked: exeFileDialog.open()
                }
            }
        }

        RuntimePicker {
            id: runtimePicker
            Layout.fillWidth: true
            twinFormLayouts: topForm
        }

        Kirigami.FormLayout {
            twinFormLayouts: runtimePicker.formLayout

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Options")
            }

            QQC2.TextField {
                id: launchOptionsField
                Kirigami.FormData.label: i18n("Launch Options (optional):")
                placeholderText: i18n("e.g ENV_VAR1=2")
            }
        }
    }

    FileDialog {
        id: exeFileDialog
        title: i18n("Select Executable")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: runtimePicker.runtimeType === "native" ? [i18n("Binaries, scripts & AppImages (*.sh *.py *.pl *.rb *.run *.bash *.zsh *.AppImage *.appimage *.desktop)"), i18n("All files (*)")] : [i18n("Executables (*.exe)"), i18n("All files (*)")]
        onAccepted: exeField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }
}
