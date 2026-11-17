import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog

    title: i18n("Updates Available")
    preferredWidth: Kirigami.Units.gridUnit * 28
    padding: Kirigami.Units.largeSpacing
    closePolicy: updateChecker.applying ? QQC2.Popup.NoAutoClose : QQC2.Popup.CloseOnEscape

    Connections {
        target: updateChecker
        function onUpdatesAvailable() {
            if (!updateChecker.applying)
                dialog.open();
        }
        function onApplyFinished() {
            dialog.close();
            applicationWindow().showPassiveNotification(i18n("Updates installed"), 3000);
        }
    }

    customFooterActions: [
        Kirigami.Action {
            text: i18n("Update")
            icon.name: "folder-download-symbolic"
            enabled: !updateChecker.applying
            onTriggered: updateChecker.applyUpdates()
        },
        Kirigami.Action {
            text: i18n("Skip These Versions")
            icon.name: "dialog-cancel-symbolic"
            enabled: !updateChecker.applying
            onTriggered: {
                updateChecker.skipUpdates();
                dialog.close();
            }
        },
        Kirigami.Action {
            text: i18n("Later")
            icon.name: "dialog-close-symbolic"
            enabled: !updateChecker.applying
            onTriggered: dialog.close()
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Newer versions of the tools Vermouth uses to run your games are available.")
        }

        Repeater {
            model: updateChecker.updates
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 0
                QQC2.Label {
                    text: modelData.name
                    font.bold: true
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WrapAnywhere
                    text: i18n("%1 → %2", modelData.installed, modelData.latest)
                    opacity: 0.75
                }
            }
        }

        DownloaderProgress {
            Layout.fillWidth: true
            visible: updateChecker.applying
            progress: updateChecker.progress
            statusText: updateChecker.statusText
        }
    }
}
