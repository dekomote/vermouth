import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: editMode ? i18n("Edit App/Game") : i18n("Add App/Game")
    preferredWidth: Kirigami.Units.gridUnit * 35
    padding: Kirigami.Units.largeSpacing
    bottomPadding: 30
    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: [
        Kirigami.Action {
            text: i18n("OK")
            icon.name: "dialog-ok"
            onTriggered: {
                if (dialog.validate()) {
                    dialog.doSave();
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

    property bool editMode: false
    property int editIndex: -1
    property string prefixBasePath

    function openForNew() {
        editMode = false;
        editIndex = -1;
        nameField.text = "";
        exeField.text = "";
        protonPrefixField.text = "";
        winePrefixField.text = "";
        launchOptionsField.text = "";
        enableLoggingCheck.checked = false;
        iconField.text = "";
        runtimePicker.reset();
        prefixBasePath = protonScanner.prefixBasePath();
        dialog.open();
    }

    function openForEdit(index) {
        editMode = true;
        editIndex = index;
        var app = appModel.getApp(index);
        nameField.text = app.name;
        exeField.text = app.exePath;
        protonPrefixField.text = app.protonPrefix;
        winePrefixField.text = app.winePrefix;
        launchOptionsField.text = app.launchOptions;
        enableLoggingCheck.checked = app.enableLogging;
        iconField.text = app.iconPath;
        prefixBasePath = protonScanner.prefixBasePath();
        runtimePicker.loadFromApp(app);
        dialog.open();
    }

    property string validationError: ""

    function validate() {
        if (nameField.text.trim() === "") {
            validationError = i18n("Name is required.");
            return false;
        }
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

    function doSave() {
        var rt = runtimePicker.runtimeType;
        var protonPath = runtimePicker.protonPath;

        if (editMode) {
            appModel.editApp(editIndex, nameField.text, exeField.text, rt, protonPath, protonPrefixField.text, runtimePicker.wineBinary, winePrefixField.text, iconField.text, launchOptionsField.text, enableLoggingCheck.checked);
        } else {
            appModel.addApp(nameField.text, exeField.text, rt, protonPath, protonPrefixField.text, runtimePicker.wineBinary, winePrefixField.text, iconField.text, launchOptionsField.text, enableLoggingCheck.checked);
        }
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

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Game")
            }

            QQC2.TextField {
                id: nameField
                Layout.topMargin: 10
                Kirigami.FormData.label: i18n("Name:")
                placeholderText: i18n("My Game")
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Executable (.exe):")
                QQC2.TextField {
                    id: exeField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/game.exe"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: exeFileDialog.open()
                }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Icon (optional):")
                QQC2.TextField {
                    id: iconField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/icon.png"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: iconFileDialog.open()
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

            RowLayout {
                visible: runtimePicker.runtimeType === "proton"
                Kirigami.FormData.label: i18n("Proton Prefix (optional):")
                QQC2.TextField {
                    id: protonPrefixField
                    Layout.fillWidth: true
                    placeholderText: dialog.prefixBasePath + "/mygame"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: prefixFolderDialog.open()
                }
            }

            RowLayout {
                visible: runtimePicker.runtimeType === "wine"
                Kirigami.FormData.label: i18n("Wine Prefix (WINEPREFIX):")
                QQC2.TextField {
                    id: winePrefixField
                    Layout.fillWidth: true
                    placeholderText: "~/.wine"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: winePrefixFolderDialog.open()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Options")
            }

            QQC2.TextField {
                id: launchOptionsField
                Kirigami.FormData.label: i18n("Launch Options (optional):")
                placeholderText: i18n("e.g. mangohud %command%")
            }

            QQC2.CheckBox {
                id: enableLoggingCheck
                text: i18n("Write output to log file")
            }
        }
    }

    FileDialog {
        id: exeFileDialog
        title: i18n("Select Executable")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Executables (*.exe)"), i18n("All files (*)")]
        onAccepted: {
            var path = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            exeField.text = path;
            if (iconField.text === "") {
                var extracted = iconExtractor.extractIcon(path);
                if (extracted !== "")
                    iconField.text = extracted;
            }
            if (nameField.text === "") {
                var parts = path.split("/");
                var filename = parts[parts.length - 1];
                nameField.text = filename.replace(/\.exe$/i, "");
            }
            var safeName = nameField.text.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase();
            var prefixBase = dialog.prefixBasePath + "/" + safeName;
            if (protonPrefixField.text === "")
                protonPrefixField.text = prefixBase;
            if (winePrefixField.text === "")
                winePrefixField.text = prefixBase;
        }
    }

    FileDialog {
        id: iconFileDialog
        title: i18n("Select Icon")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("Images (*.png *.svg *.ico *.jpg)"), i18n("All files (*)")]
        onAccepted: iconField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }

    FolderDialog {
        id: prefixFolderDialog
        title: i18n("Select Proton Prefix Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: protonPrefixField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: winePrefixFolderDialog
        title: i18n("Select Wine Prefix Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: winePrefixField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }
}
