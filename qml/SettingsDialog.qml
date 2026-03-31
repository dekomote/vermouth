import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.PromptDialog {
    id: dialog
    title: i18n("Settings")
    preferredWidth: Kirigami.Units.gridUnit * 30
    padding: Kirigami.Units.largeSpacing
    bottomPadding: 30
    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    onAccepted: settingsManager.setDefaultPrefixDir(prefixDirField.text)

    function openDialog() {
        prefixDirField.text = settingsManager.defaultPrefixDir;
        pathsModel.clear();
        var paths = settingsManager.extraProtonPaths;
        for (var i = 0; i < paths.length; i++) {
            pathsModel.append({
                "path": paths[i]
            });
        }
        dialog.open();
    }

    ListModel {
        id: pathsModel
    }

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing

        Kirigami.FormLayout {

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18n("Prefixes")
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Default Prefix Directory:")
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
                Kirigami.FormData.label: i18n("Extra Proton Scan Paths")
            }

            ColumnLayout {
                Kirigami.FormData.label: i18n("Directories to scan for Proton installations (in addition to Steam and local paths).")
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

    FolderDialog {
        id: prefixDirFolderDialog
        title: i18n("Select Default Prefix Directory")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: prefixDirField.text = selectedFolder.toString().replace("file://", "")
    }

    FolderDialog {
        id: protonPathFolderDialog
        title: i18n("Select Proton Scan Directory")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: {
            var path = selectedFolder.toString().replace("file://", "");
            settingsManager.addExtraProtonPath(path);
            pathsModel.append({
                "path": path
            });
        }
    }
}
