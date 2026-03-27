import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: editMode ? "Edit App/Game" : "Add App/Game"
    preferredWidth: Kirigami.Units.gridUnit * 30
    padding: Kirigami.Units.largeSpacing
    modal: true
    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: [
        Kirigami.Action {
            text: "OK"
            icon.name: "dialog-ok"
            onTriggered: {
                if (dialog.validate()) {
                    dialog.doSave()
                    dialog.close()
                }
            }
        },
        Kirigami.Action {
            text: "Cancel"
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    property bool editMode: false
    property int editIndex: -1

    function openForNew() {
        editMode = false
        editIndex = -1
        nameField.text = ""
        exeField.text = ""
        runtimeCombo.currentIndex = 0
        protonCombo.currentIndex = -1
        protonPrefixField.text = ""
        wineBinaryField.text = ""
        winePrefixField.text = ""
        launchOptionsField.text = ""
        enableLoggingCheck.checked = false
        iconField.text = ""
        refreshProton()
        dialog.open()
    }

    function openForEdit(index) {
        editMode = true
        editIndex = index
        var app = appModel.getApp(index)
        nameField.text = app.name
        exeField.text = app.exePath
        runtimeCombo.currentIndex = app.runtimeType === "proton" ? 0 : 1
        protonPrefixField.text = app.protonPrefix
        wineBinaryField.text = app.wineBinary
        winePrefixField.text = app.winePrefix
        launchOptionsField.text = app.launchOptions
        enableLoggingCheck.checked = app.enableLogging
        iconField.text = app.iconPath
        refreshProton()

        if (app.runtimeType === "proton") {
            for (var i = 0; i < protonModel.count; i++) {
                if (protonModel.get(i).path === app.protonPath) {
                    protonCombo.currentIndex = i
                    break
                }
            }
        }
        dialog.open()
    }

    function refreshProton() {
        protonModel.clear()
        var versions = protonScanner.findProtonVersions()
        for (var i = 0; i < versions.length; i++) {
            var parts = versions[i].split("/")
            protonModel.append({
                "label": parts[parts.length - 1],
                "path": versions[i]
            })
        }
    }

    property string validationError: ""

    function validate() {
        if (nameField.text.trim() === "") {
            validationError = "Name is required."
            return false
        }
        if (exeField.text.trim() === "") {
            validationError = "Executable path is required."
            return false
        }
        if (runtimeCombo.currentIndex === 0) {
            if (protonCombo.currentIndex < 0 || protonCombo.currentIndex >= protonModel.count) {
                validationError = "Please select a Proton version."
                return false
            }
        } else {
            if (wineBinaryField.text.trim() === "") {
                validationError = "Wine binary path is required."
                return false
            }
        }
        validationError = ""
        return true
    }

    function doSave() {
        var rt = runtimeCombo.currentIndex === 0 ? "proton" : "wine"
        var protonPath = ""
        if (protonCombo.currentIndex >= 0 && protonCombo.currentIndex < protonModel.count)
            protonPath = protonModel.get(protonCombo.currentIndex).path

        if (editMode) {
            appModel.editApp(editIndex, nameField.text, exeField.text, rt,
                             protonPath, protonPrefixField.text,
                             wineBinaryField.text, winePrefixField.text,
                             iconField.text, launchOptionsField.text,
                             enableLoggingCheck.checked)
        } else {
            appModel.addApp(nameField.text, exeField.text, rt,
                            protonPath, protonPrefixField.text,
                            wineBinaryField.text, winePrefixField.text,
                            iconField.text, launchOptionsField.text,
                            enableLoggingCheck.checked)
        }
    }

    ListModel { id: protonModel }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            text: dialog.validationError
            visible: dialog.validationError !== ""
        }

        Kirigami.FormLayout {

            QQC2.TextField {
                id: nameField
                Kirigami.FormData.label: "Name:"
                placeholderText: "My Game"
            }

            RowLayout {
                Kirigami.FormData.label: "Executable (.exe):"
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

            QQC2.ComboBox {
                id: runtimeCombo
                Kirigami.FormData.label: "Runtime:"
                model: ["Proton", "Wine"]
            }

            // Proton settings
            RowLayout {
                visible: runtimeCombo.currentIndex === 0
                Kirigami.FormData.label: "Proton Version:"
                QQC2.ComboBox {
                    id: protonCombo
                    Layout.fillWidth: true
                    model: protonModel
                    textRole: "label"
                }
                QQC2.Button {
                    icon.name: "folder-open"
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: "Open local Protons folder (" + protonScanner.localProtonPath() + ")"
                    onClicked: Qt.openUrlExternally("file://" + protonScanner.localProtonPath())
                }
                QQC2.Button {
                    icon.name: "view-refresh"
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: "Refresh Proton versions"
                    onClicked: dialog.refreshProton()
                }
            }

            RowLayout {
                visible: runtimeCombo.currentIndex === 0
                Kirigami.FormData.label: "Proton Prefix (optional):"
                QQC2.TextField {
                    id: protonPrefixField
                    Layout.fillWidth: true
                    placeholderText: "~/.local/share/vermouth/prefixes/mygame"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: prefixFolderDialog.open()
                }
            }

            // Wine settings
            RowLayout {
                visible: runtimeCombo.currentIndex === 1
                Kirigami.FormData.label: "Wine Binary:"
                QQC2.TextField {
                    id: wineBinaryField
                    Layout.fillWidth: true
                    placeholderText: "/usr/bin/wine"
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: wineBinaryDialog.open()
                }
            }

            RowLayout {
                visible: runtimeCombo.currentIndex === 1
                Kirigami.FormData.label: "Wine Prefix (WINEPREFIX):"
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

            QQC2.TextField {
                id: launchOptionsField
                Kirigami.FormData.label: "Launch Options (optional):"
                placeholderText: "e.g. mangohud %command%"
            }

            RowLayout {
                Kirigami.FormData.label: "Icon (optional):"
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

            QQC2.CheckBox {
                id: enableLoggingCheck
                text: "Write output to log file"
            }
        }
    }

    FileDialog {
        id: exeFileDialog
        title: "Select Executable"
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: ["Executables (*.exe)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "")
            exeField.text = path
            if (iconField.text === "") {
                var extracted = iconExtractor.extractIcon(path)
                if (extracted !== "")
                    iconField.text = extracted
            }
            if (nameField.text === "") {
                var parts = path.split("/")
                var filename = parts[parts.length - 1]
                nameField.text = filename.replace(/\.exe$/i, "")
            }
            var safeName = nameField.text.replace(/[^a-zA-Z0-9_-]/g, "_").toLowerCase()
            var prefixBase = protonScanner.prefixBasePath() + "/" + safeName
            if (protonPrefixField.text === "")
                protonPrefixField.text = prefixBase
            if (winePrefixField.text === "")
                winePrefixField.text = prefixBase
        }
    }
    FileDialog {
        id: wineBinaryDialog
        title: "Select Wine Binary"
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: wineBinaryField.text = selectedFile.toString().replace("file://", "")
    }
    FileDialog {
        id: iconFileDialog
        title: "Select Icon"
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: ["Images (*.png *.svg *.ico *.jpg)", "All files (*)"]
        onAccepted: iconField.text = selectedFile.toString().replace("file://", "")
    }
    FolderDialog {
        id: prefixFolderDialog
        title: "Select Proton Prefix Directory"
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: protonPrefixField.text = selectedFolder.toString().replace("file://", "")
    }
    FolderDialog {
        id: winePrefixFolderDialog
        title: "Select Wine Prefix Directory"
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: winePrefixField.text = selectedFolder.toString().replace("file://", "")
    }
}
