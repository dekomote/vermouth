import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root
    spacing: Kirigami.Units.mediumSpacing

    readonly property string runtimeType: runtimeCombo.currentIndex === 0 ? "proton" : "wine"
    readonly property string protonPath: protonCombo.currentIndex >= 0 && protonCombo.currentIndex < protonModel.count ? protonModel.get(protonCombo.currentIndex).path : ""
    readonly property alias wineBinary: wineBinaryField.text
    property string sectionLabel: i18n("Runtime")

    function reset() {
        refreshProton();
        loadFromSettings();
    }

    function loadFromSettings() {
        var rt = settingsManager.defaultRuntimeType;
        runtimeCombo.currentIndex = rt === "wine" ? 1 : 0;
        wineBinaryField.text = settingsManager.defaultWineBinary;

        var pp = settingsManager.defaultProtonPath;
        protonCombo.currentIndex = -1;
        if (pp !== "") {
            for (var i = 0; i < protonModel.count; i++) {
                if (protonModel.get(i).path === pp) {
                    protonCombo.currentIndex = i;
                    break;
                }
            }
        }
    }

    function saveToSettings() {
        settingsManager.setDefaultRuntimeType(runtimeType);
        settingsManager.setDefaultProtonPath(protonPath);
        settingsManager.setDefaultWineBinary(wineBinary);
    }

    function loadFromApp(app) {
        runtimeCombo.currentIndex = app.runtimeType === "proton" ? 0 : 1;
        wineBinaryField.text = app.wineBinary;
        refreshProton();

        if (app.runtimeType === "proton") {
            for (var i = 0; i < protonModel.count; i++) {
                if (protonModel.get(i).path === app.protonPath) {
                    protonCombo.currentIndex = i;
                    break;
                }
            }
        }
    }

    function validate() {
        if (runtimeCombo.currentIndex === 0) {
            if (protonCombo.currentIndex < 0 || protonCombo.currentIndex >= protonModel.count)
                return i18n("Please select a Proton version.");
        } else {
            if (wineBinaryField.text.trim() === "")
                return i18n("Wine binary path is required.");
        }
        return "";
    }

    function refreshProton() {
        protonModel.clear();
        var versions = protonScanner.findProtonVersions();
        for (var i = 0; i < versions.length; i++) {
            var parts = versions[i].split("/");
            protonModel.append({
                "label": parts[parts.length - 1],
                "path": versions[i]
            });
        }
    }

    ListModel {
        id: protonModel
    }

    Connections {
        target: protonDownloader
        function onFinished() {
            root.refreshProton();
        }
    }

    Kirigami.FormLayout {

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: root.sectionLabel
        }

        QQC2.ComboBox {
            id: runtimeCombo
            Kirigami.FormData.label: i18n("Runtime:")
            model: ["Proton", "Wine"]
        }

        RowLayout {
            visible: runtimeCombo.currentIndex === 0
            Kirigami.FormData.label: i18n("Proton Version:")
            QQC2.ComboBox {
                id: protonCombo
                Layout.fillWidth: true
                model: protonModel
                textRole: "label"
                displayText: protonModel.count === 0 ? i18n("No Proton versions found. Download GE Proton to get started - no Steam or manual setup needed.") : currentText
                QQC2.ToolTip.visible: hovered && protonModel.count === 0
                QQC2.ToolTip.text: protonModel.count === 0 ? i18n("No Proton versions found. Download GE Proton to get started - no Steam or manual setup needed.") : ""
            }
            QQC2.Button {
                icon.name: "folder-open"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Open Vermouth Proton folder (%1)", protonScanner.localProtonPath())
                onClicked: Qt.openUrlExternally("file://" + protonScanner.localProtonPath())
            }
            QQC2.Button {
                icon.name: "view-refresh"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Refresh Proton versions")
                onClicked: root.refreshProton()
            }
            QQC2.Button {
                icon.name: "download"
                enabled: !protonDownloader.busy
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: protonDownloader.statusText ? protonDownloader.statusText : i18n("Download latest GE Proton")
                onClicked: protonDownloader.downloadLatest()
            }
        }

        RowLayout {
            visible: runtimeCombo.currentIndex === 1
            Kirigami.FormData.label: i18n("Wine Binary:")
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
    }

    FileDialog {
        id: wineBinaryDialog
        title: i18n("Select Wine Binary")
        currentFolder: "file://" + protonScanner.homePath()
        onAccepted: wineBinaryField.text = decodeURIComponent(selectedFile.toString().replace("file://", ""))
    }
}
