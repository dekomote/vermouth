import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialog
    title: editMode ? "Edit App/Game" : "Add App/Game"
    modal: true
    width: 520
    height: 600
    anchors.centerIn: parent
    standardButtons: Dialog.NoButton

    footer: DialogButtonBox {
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: "OK"
            highlighted: true
            onClicked: {
                if (dialog.validate()) {
                    dialog.doSave()
                    dialog.close()
                }
            }
        }
    }

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

    contentItem: Flickable {
        contentHeight: formLayout.implicitHeight
        clip: true

        ColumnLayout {
            id: formLayout
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 10

            Label {
                text: dialog.validationError
                color: "red"
                font.pixelSize: 12
                visible: dialog.validationError !== ""
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Label { text: "Name"; font.pixelSize: 12 }
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "My Game"
            }

            Label { text: "Executable (.exe)"; font.pixelSize: 12 }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: exeField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/game.exe"
                }
                Button {
                    text: "Browse"
                    onClicked: exeFileDialog.open()
                }
            }

            Label { text: "Runtime"; font.pixelSize: 12 }
            ComboBox {
                id: runtimeCombo
                Layout.fillWidth: true
                model: ["Proton", "Wine"]
            }

            // Proton settings
            ColumnLayout {
                visible: runtimeCombo.currentIndex === 0
                Layout.fillWidth: true
                spacing: 10

                Label { text: "Proton Version"; font.pixelSize: 12 }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox {
                        id: protonCombo
                        Layout.fillWidth: true
                        model: protonModel
                        textRole: "label"
                    }
                    Button {
                        icon.name: "folder-open"
                        ToolTip.visible: hovered
                        ToolTip.text: "Open local Protons folder (" + protonScanner.localProtonPath() + ")"
                        onClicked: Qt.openUrlExternally("file://" + protonScanner.localProtonPath())
                    }
                    Button {
                        icon.name: "view-refresh"
                        ToolTip.visible: hovered
                        ToolTip.text: "Refresh Proton versions"
                        onClicked: dialog.refreshProton()
                    }
                }

                Label { text: "Proton Prefix (optional)"; font.pixelSize: 12 }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: protonPrefixField
                        Layout.fillWidth: true
                        placeholderText: "~/.local/share/vermouth/prefixes/mygame"
                    }
                    Button {
                        text: "Browse"
                        onClicked: prefixFolderDialog.open()
                    }
                }
            }

            // Wine settings
            ColumnLayout {
                visible: runtimeCombo.currentIndex === 1
                Layout.fillWidth: true
                spacing: 10

                Label { text: "Wine Binary"; font.pixelSize: 12 }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: wineBinaryField
                        Layout.fillWidth: true
                        placeholderText: "/usr/bin/wine"
                    }
                    Button {
                        text: "Browse"
                        onClicked: wineBinaryDialog.open()
                    }
                }

                Label { text: "Wine Prefix (WINEPREFIX)"; font.pixelSize: 12 }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: winePrefixField
                        Layout.fillWidth: true
                        placeholderText: "~/.wine"
                    }
                    Button {
                        text: "Browse"
                        onClicked: winePrefixFolderDialog.open()
                    }
                }
            }

            Label { text: "Launch Options (optional)"; font.pixelSize: 12 }
            TextField {
                id: launchOptionsField
                Layout.fillWidth: true
                placeholderText: "e.g. mangohud %command%"
            }


            Label { text: "Icon (optional)"; font.pixelSize: 12 }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: iconField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/icon.png"
                }
                Button {
                    text: "Browse"
                    onClicked: iconFileDialog.open()
                }
            }

            CheckBox {
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
            // Auto-extract icon from exe
            if (iconField.text === "") {
                var extracted = iconExtractor.extractIcon(path)
                if (extracted !== "")
                    iconField.text = extracted
            }
            // Auto-fill name from filename if empty
            if (nameField.text === "") {
                var parts = path.split("/")
                var filename = parts[parts.length - 1]
                nameField.text = filename.replace(/\.exe$/i, "")
            }
            // Auto-generate prefix path
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
