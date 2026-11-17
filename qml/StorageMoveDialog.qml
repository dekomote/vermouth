import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Asks what to do with the files in a storage folder that was just changed: move them and update the games, or only change the folder.
Kirigami.Dialog {
    id: dialog

    // [{kind, label, typed, oldPath, newPath}]
    property var changes: []
    property var queue: []
    property var current: null
    property string errorText: ""
    property bool moveFiles: true

    signal cancelled
    signal applyDirect(var change)

    title: i18n("Change Folders")
    preferredWidth: Kirigami.Units.gridUnit * 30
    padding: Kirigami.Units.largeSpacing
    closePolicy: storageMigrator.busy ? QQC2.Popup.NoAutoClose : QQC2.Popup.CloseOnEscape

    function next() {
        if (queue.length === 0) {
            current = null;
            dialog.close();
            applicationWindow().showPassiveNotification(moveFiles ? i18n("Folders moved and games updated") : i18n("Games updated"), 3000);
            return;
        }
        current = queue[0];
        queue = queue.slice(1);
        storageMigrator.migrate(current.kind, current.oldPath, current.newPath, moveFiles);
    }

    Connections {
        target: storageMigrator
        function onFinished(kind, ok, message) {
            if (!dialog.current)
                return;
            if (!ok) {
                dialog.errorText = message;
                dialog.queue = [];
                dialog.current = null;
                return;
            }
            dialog.applyDirect(dialog.current);
            dialog.next();
        }
    }

    customFooterActions: [
        Kirigami.Action {
            text: i18n("Move Files and Games")
            icon.name: "folder-move-symbolic"
            enabled: !storageMigrator.busy
            onTriggered: {
                dialog.errorText = "";
                dialog.moveFiles = true;
                dialog.queue = dialog.changes.slice();
                dialog.next();
            }
        },
        Kirigami.Action {
            text: i18n("Move Only Games")
            icon.name: "document-edit-symbolic"
            enabled: !storageMigrator.busy
            onTriggered: {
                dialog.errorText = "";
                dialog.moveFiles = false;
                dialog.queue = dialog.changes.slice();
                dialog.next();
            }
        },
        Kirigami.Action {
            text: i18n("Don't Move Anything")
            icon.name: "folder-symbolic"
            enabled: !storageMigrator.busy
            onTriggered: {
                for (var i = 0; i < dialog.changes.length; i++)
                    dialog.applyDirect(dialog.changes[i]);
                dialog.close();
            }
        },
        Kirigami.Action {
            text: i18n("Cancel")
            icon.name: "dialog-cancel-symbolic"
            enabled: !storageMigrator.busy
            onTriggered: {
                dialog.cancelled();
                dialog.close();
            }
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("What should happen to the files in the old folders?\n\nMove Files and Games moves the files and updates the games and settings that point to them. Move Only Games updates the games and settings but leaves the files alone, for when you already moved them yourself. Don't Move Anything only changes the folder. Don't move files while a game is running.")
        }

        Repeater {
            model: dialog.changes
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 0
                QQC2.Label {
                    text: modelData.label
                    font.bold: true
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WrapAnywhere
                    text: i18n("%1 → %2", modelData.oldPath, modelData.newPath)
                    opacity: 0.75
                }
            }
        }

        QQC2.BusyIndicator {
            visible: storageMigrator.busy
            running: visible
            Layout.alignment: Qt.AlignHCenter
        }

        QQC2.Label {
            visible: dialog.errorText !== ""
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Kirigami.Theme.negativeTextColor
            text: dialog.errorText
        }
    }
}
