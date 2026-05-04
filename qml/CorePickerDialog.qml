import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.PromptDialog {
    id: dialog
    property string platformSlug: ""
    property var pendingRom: null
    property int appIndex: -1
    property bool launchAfterPick: true
    property var availableCores: []
    property string customCorePath: ""

    function resolvedPath() {
        if (customCorePath !== "")
            return customCorePath;
        if (availableCores.length > 0 && coreCombo.currentIndex >= 0)
            return availableCores[coreCombo.currentIndex];
        return "";
    }

    title: pendingRom ? i18n("Select core for \"%1\"", pendingRom.name) : i18n("Select RetroArch Core")
    subtitle: platformSlug !== "" ? i18n("Platform: %1", platformSlug) : ""
    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel

    onOpened: {
        availableCores = launcher.availableCoresForPlatform(platformSlug);
        customCorePath = "";
        customCoreField.text = "";
        coreCombo.currentIndex = availableCores.length > 0 ? 0 : -1;
    }
    onClosed: {
        customCorePath = "";
        availableCores = [];
        pendingRom = null;
        appIndex = -1;
    }
    onAccepted: {
        var path = resolvedPath();
        if (path === "")
            return;
        if (appIndex >= 0) {
            var app = appModel.getApp(appIndex);
            app.customCorePath = path;
            appModel.editApp(appIndex, app);
        } else if (pendingRom) {
            settingsManager.setRommGameCore(pendingRom.romId, path);
            if (launchAfterPick)
                launcher.launchRom(pendingRom);
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.ComboBox {
            id: coreCombo
            Layout.fillWidth: true
            visible: dialog.availableCores.length > 0
            model: dialog.availableCores.map(function (p) {
                return p.split("/").pop();
            })
            onActivated: {
                dialog.customCorePath = "";
                customCoreField.text = "";
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: dialog.availableCores.length === 0
            type: Kirigami.MessageType.Warning
            text: i18n("No cores found automatically for this platform.")
        }

        QQC2.Label {
            text: dialog.availableCores.length > 0 ? i18n("Or use a custom core:") : i18n("Browse for a core file:")
            opacity: 0.7
        }

        RowLayout {
            Layout.fillWidth: true
            QQC2.TextField {
                id: customCoreField
                Layout.fillWidth: true
                placeholderText: i18n("Path to .so core file")
                onTextEdited: dialog.customCorePath = text.trim()
            }
            QQC2.Button {
                icon.name: "document-open"
                onClicked: coreFileDialog.open()
            }
        }
    }

    FileDialog {
        id: coreFileDialog
        title: i18n("Select RetroArch Core")
        nameFilters: [i18n("RetroArch cores (*.so)"), i18n("All files (*)")]
        onAccepted: {
            var path = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            dialog.customCorePath = path;
            customCoreField.text = path;
        }
    }
}
