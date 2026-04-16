import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.PromptDialog {
    id: dialog
    title: i18n("Settings")
    preferredWidth: Kirigami.Units.gridUnit * 30
    bottomPadding: 30
    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    onAccepted: {
        settingsManager.setUmuPath(umuPathField.text);
        settingsManager.setDefaultPrefixDir(prefixDirField.text);
        defaultRuntimePicker.saveToSettings();
    }

    function openDialog() {
        umuPathField.text = settingsManager.umuPath;
        prefixDirField.text = settingsManager.defaultPrefixDir;
        pathsModel.clear();
        var paths = settingsManager.extraProtonPaths;
        for (var i = 0; i < paths.length; i++) {
            pathsModel.append({
                "path": paths[i]
            });
        }
        defaultRuntimePicker.reset();
        dialog.open();
    }

    ListModel {
        id: pathsModel
    }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RuntimePicker {
            id: defaultRuntimePicker
            Layout.fillWidth: true
            sectionLabel: i18n("Default Runtime")
        }

        Kirigami.FormLayout {
            twinFormLayouts: defaultRuntimePicker.formLayout

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("umu-launcher")
            }

            QQC2.Label {
                Kirigami.FormData.label: ""
                text: i18n("umu-launcher runs Proton through the Steam Runtime (pressure-vessel), which significantly improves game compatibility - especially for games with video cutscenes or anti-cheat. Strongly recommended.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 26
                font.italic: true
                opacity: 0.8
            }

            RowLayout {
                Kirigami.FormData.label: i18n("umu-run path:")
                QQC2.TextField {
                    id: umuPathField
                    Layout.fillWidth: true
                    placeholderText: i18n("Auto-detect (umu-run in PATH)")
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: umuFilePicker.open()
                }
                QQC2.Button {
                    icon.name: "download"
                    enabled: !umuDownloader.busy
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: umuDownloader.statusText ? umuDownloader.statusText : i18n("Download latest umu-launcher")
                    onClicked: umuDownloader.downloadLatest()
                }
            }

            Connections {
                target: settingsManager
                function onUmuPathChanged() {
                    umuPathField.text = settingsManager.umuPath;
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Prefixes")
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Default Prefix Folder:")
                QQC2.TextField {
                    id: prefixDirField
                    Layout.fillWidth: true
                    placeholderText: protonScanner.prefixBasePath()
                }
                QQC2.Button {
                    icon.name: "document-open"
                    onClicked: prefixDirFolderDialog.open()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Vermouth Proton Folder")
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Download GE Proton to run most games and apps - no Steam or manual setup needed.")
                QQC2.Button {
                    icon.name: "folder-open"
                    text: i18n("Open Vermouth Proton folder")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: protonScanner.localProtonPath()
                    onClicked: Qt.openUrlExternally("file://" + protonScanner.localProtonPath())
                }
                QQC2.Button {
                    text: i18n("Download Latest GE Proton")
                    icon.name: "download"
                    enabled: !protonDownloader.busy
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: protonDownloader.statusText ? protonDownloader.statusText : i18n("Download latest GE Proton")
                    onClicked: protonDownloader.downloadLatest()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Extra Proton Scan Paths")
            }

            ColumnLayout {
                Kirigami.FormData.label: i18n("Folders to scan for Proton installations (in addition to Steam and local paths).")
                Layout.fillWidth: true
                Repeater {
                    model: pathsModel
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        QQC2.TextField {
                            text: model.path
                            Layout.fillWidth: true
                            readOnly: true
                        }
                        QQC2.Button {
                            icon.name: "list-remove"
                            onClicked: {
                                settingsManager.removeExtraProtonPath(index);
                                pathsModel.remove(index);
                            }
                        }
                    }
                }

                QQC2.Button {
                    text: i18n("Add Path...")
                    icon.name: "list-add"
                    onClicked: protonPathFolderDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: umuFilePicker
        title: i18n("Select umu-run binary")
        currentFolder: umuPathField.text !== "" ? "file://" + umuPathField.text.substring(0, umuPathField.text.lastIndexOf("/")) : "file://" + protonScanner.homePath()
        onAccepted: umuPathField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }

    FolderDialog {
        id: prefixDirFolderDialog
        title: i18n("Select Default Prefix Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: prefixDirField.text = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }

    FolderDialog {
        id: protonPathFolderDialog
        title: i18n("Select Proton Scan Folder")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: {
            var path = decodeURIComponent(selectedFolder.toString().replace("file://", ""));
            settingsManager.addExtraProtonPath(path);
            pathsModel.append({
                "path": path
            });
        }
    }
}
