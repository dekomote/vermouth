import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root
    spacing: Kirigami.Units.mediumSpacing

    readonly property string runtimeType: runtimeCombo.currentValue
    readonly property string protonPath: protonCombo.currentIndex >= 0 && protonCombo.currentIndex < protonModel.count ? protonModel.get(protonCombo.currentIndex).path : ""
    readonly property string wineBinary: wineCombo.currentIndex >= 0 && wineCombo.currentIndex < wineModel.count ? wineModel.get(wineCombo.currentIndex).path : ""
    readonly property string uzdoomPath: uzdoomPathField.text.trim()
    property string sectionLabel: i18n("Runtime")
    property alias formLayout: formLayout
    property var twinFormLayouts
    property bool autoSaveDefaults: false
    property bool showDefaultOption: false
    property bool protonWineOnly: false

    readonly property string resolvedRuntimeType: runtimeType === "default" ? settingsManager.defaultRuntimeType : runtimeType
    readonly property string resolvedProtonPath: runtimeType === "default" ? settingsManager.defaultProtonPath : protonPath
    readonly property string resolvedWineBinary: runtimeType === "default" ? settingsManager.defaultWineBinary : wineBinary

    readonly property bool defaultRuntimeAvailable: {
        var t = settingsManager.defaultRuntimeType;
        if (t === "proton")
            return settingsManager.defaultProtonPath !== "" && protonScanner.isInstalled(settingsManager.defaultProtonPath);
        if (t === "wine")
            return settingsManager.defaultWineBinary !== "" && wineScanner.isInstalled(settingsManager.defaultWineBinary);
        return false;
    }

    function rebuildRuntimeOptions() {
        var prevValue = runtimeCombo.currentValue;
        runtimeOptions.clear();
        if (root.showDefaultOption && root.defaultRuntimeAvailable)
            runtimeOptions.append({
                "label": i18n("Default Runtime"),
                "key": "default"
            });
        for (var i = 0; i < runtimeModel.rowCount(); i++) {
            var entry = runtimeModel.get(i);
            if (entry.key !== "default" && (!root.protonWineOnly || entry.key === "proton" || entry.key === "wine"))
                runtimeOptions.append({
                    "label": entry.label,
                    "key": entry.key
                });
        }
        if (prevValue !== undefined && prevValue !== null && prevValue !== "")
            setRuntimeType(prevValue);
    }

    function reset() {
        refreshProton();
        refreshWine();
        loadFromSettings();
    }

    function setRuntimeType(type) {
        for (let i = 0; i < runtimeCombo.count; i++) {
            if (runtimeCombo.valueAt(i) === type) {
                runtimeCombo.currentIndex = i;
                return;
            }
        }
    }

    function loadFromSettings() {
        rebuildRuntimeOptions();
        if (root.showDefaultOption && root.defaultRuntimeAvailable)
            setRuntimeType("default");
        else
            setRuntimeType(settingsManager.defaultRuntimeType);
        uzdoomPathField.text = settingsManager.uzdoomPath;

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

        var wb = settingsManager.defaultWineBinary;
        wineCombo.currentIndex = -1;
        if (wb !== "") {
            for (var i = 0; i < wineModel.count; i++) {
                if (wineModel.get(i).path === wb) {
                    wineCombo.currentIndex = i;
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
        rebuildRuntimeOptions();
        setRuntimeType(app.runtimeType);
        refreshProton();
        refreshWine();
        uzdoomPathField.text = app.uzdoomPath !== "" ? app.uzdoomPath : settingsManager.uzdoomPath;

        if (app.runtimeType === "proton") {
            for (var i = 0; i < protonModel.count; i++) {
                if (protonModel.get(i).path === app.protonPath) {
                    protonCombo.currentIndex = i;
                    break;
                }
            }
        } else if (app.runtimeType === "wine") {
            for (var i = 0; i < wineModel.count; i++) {
                if (wineModel.get(i).path === app.wineBinary) {
                    wineCombo.currentIndex = i;
                    break;
                }
            }
        }
    }

    function validate() {
        if (runtimeCombo.currentValue === "proton") {
            if (protonCombo.currentIndex < 0 || protonCombo.currentIndex >= protonModel.count)
                return i18n("Please select a Proton version.");
        } else if (runtimeCombo.currentValue === "wine") {
            if (wineCombo.currentIndex < 0 || wineCombo.currentIndex >= wineModel.count)
                return i18n("Please select a Wine version.");
        } else if (runtimeCombo.currentValue === "uzdoom") {
            if (uzdoomPathField.text.trim() === "")
                return i18n("Please select or download the UZDOOM AppImage.");
        } else if (runtimeCombo.currentValue === "default") {
            if (!root.defaultRuntimeAvailable)
                return i18n("The default runtime is not fully configured in Settings.");
        }
        return "";
    }

    function refreshProton() {
        var prevPath = "";
        if (protonCombo.currentIndex >= 0 && protonCombo.currentIndex < protonModel.count)
            prevPath = protonModel.get(protonCombo.currentIndex).path;
        protonModel.clear();
        var versions = protonScanner.findProtonVersions();
        for (let i = 0; i < versions.length; i++) {
            let parts = versions[i].split("/");
            protonModel.append({
                "label": parts[parts.length - 1],
                "path": versions[i]
            });
        }
        var defaultPath = settingsManager.defaultProtonPath;
        if (defaultPath !== "" && !protonScanner.isInstalled(defaultPath))
            settingsManager.setDefaultProtonPath("");
        for (let i = 0; i < protonModel.count; i++) {
            if (protonModel.get(i).path === prevPath) {
                protonCombo.currentIndex = i;
                return;
            }
        }
        protonCombo.currentIndex = -1;
    }

    function refreshWine() {
        var prevPath = "";
        if (wineCombo.currentIndex >= 0 && wineCombo.currentIndex < wineModel.count)
            prevPath = wineModel.get(wineCombo.currentIndex).path;
        wineModel.clear();
        var versions = wineScanner.findWineVersions();
        for (let i = 0; i < versions.length; i++) {
            wineModel.append({
                "label": versions[i].label,
                "path": versions[i].path
            });
        }
        var defaultBinary = settingsManager.defaultWineBinary;
        if (defaultBinary !== "" && !wineScanner.isInstalled(defaultBinary))
            settingsManager.setDefaultWineBinary("");
        for (let i = 0; i < wineModel.count; i++) {
            if (wineModel.get(i).path === prevPath) {
                wineCombo.currentIndex = i;
                return;
            }
        }
        wineCombo.currentIndex = -1;
    }

    ListModel {
        id: protonModel
    }

    ListModel {
        id: wineModel
    }

    ListModel {
        id: runtimeOptions
    }

    Connections {
        target: settingsManager
        function onDefaultRuntimeChanged() {
            root.rebuildRuntimeOptions();
        }
    }

    Connections {
        target: uzdoomDownloader
        function onFinished(path) {
            uzdoomPathField.text = path;
        }
    }

    Connections {
        target: protonDownloader
        function onFinished(path) {
            root.refreshProton();
            if (settingsManager.defaultProtonPath === "" && path !== "") {
                settingsManager.setDefaultProtonPath(path);
                for (var i = 0; i < protonModel.count; i++) {
                    if (protonModel.get(i).path === path) {
                        protonCombo.currentIndex = i;
                        break;
                    }
                }
            }
        }
    }

    Connections {
        target: wineDownloader
        function onFinished(path) {
            root.refreshWine();
            if (settingsManager.defaultWineBinary === "" && path !== "") {
                settingsManager.setDefaultWineBinary(path);
                for (var i = 0; i < wineModel.count; i++) {
                    if (wineModel.get(i).path === path) {
                        wineCombo.currentIndex = i;
                        break;
                    }
                }
            }
        }
    }

    Kirigami.FormLayout {
        id: formLayout
        twinFormLayouts: root.twinFormLayouts

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: root.sectionLabel
        }

        QQC2.ComboBox {
            id: runtimeCombo
            Kirigami.FormData.label: i18n("Runtime:")
            Layout.fillWidth: true
            model: runtimeOptions
            textRole: "label"
            valueRole: "key"
            Layout.minimumWidth: 400
            onActivated: {
                if (root.autoSaveDefaults && currentValue !== "")
                    settingsManager.setDefaultRuntimeType(currentValue);
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: runtimeCombo.currentValue === "proton"
            Kirigami.FormData.label: i18n("Proton Version:")
            QQC2.ComboBox {
                id: protonCombo
                Layout.fillWidth: true
                model: protonModel
                textRole: "label"
                displayText: protonModel.count === 0 ? i18n("No Proton versions found. Download GE Proton to get started - no Steam or manual setup needed.") : currentText
                QQC2.ToolTip.visible: hovered && protonModel.count === 0
                QQC2.ToolTip.text: protonModel.count === 0 ? i18n("No Proton versions found. Download GE Proton to get started - no Steam or manual setup needed.") : ""
                onActivated: {
                    if (currentIndex >= 0 && (root.autoSaveDefaults || settingsManager.defaultProtonPath === ""))
                        settingsManager.setDefaultProtonPath(protonModel.get(currentIndex).path);
                }
            }
            QQC2.ToolButton {
                icon.name: "folder-open-symbolic"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Open Vermouth Proton folder (%1)", protonScanner.localProtonPath())
                onClicked: Qt.openUrlExternally("file://" + protonScanner.localProtonPath())
            }
            QQC2.ToolButton {
                icon.name: "view-refresh-symbolic"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Refresh Proton versions")
                onClicked: root.refreshProton()
            }
            QQC2.ToolButton {
                id: protonDownloadButton
                icon.name: "folder-download-symbolic"
                enabled: !protonDownloader.busy
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: protonDownloader.statusText ? protonDownloader.statusText : i18n("Download Proton build")
                onClicked: protonDownloadMenu.popup()

                QQC2.Menu {
                    id: protonDownloadMenu
                    QQC2.MenuItem {
                        text: i18n("Download latest GE Proton")
                        enabled: !protonDownloader.busy
                        onTriggered: protonDownloader.downloadLatest("ge")
                    }
                    QQC2.MenuItem {
                        text: i18n("Download latest Proton-CachyOS")
                        enabled: !protonDownloader.busy
                        onTriggered: protonDownloader.downloadLatest("cachyos")
                    }
                }
            }
        }

        DownloaderProgress {
            Layout.fillWidth: true
            Kirigami.FormData.label: ""
            visible: runtimeCombo.currentValue === "proton" && protonDownloader.busy
            progress: protonDownloader.progress
            statusText: protonDownloader.statusText
        }

        RowLayout {
            Layout.fillWidth: true
            visible: runtimeCombo.currentValue === "wine"
            Kirigami.FormData.label: i18n("Wine Version:")
            QQC2.ComboBox {
                id: wineCombo
                Layout.fillWidth: true
                model: wineModel
                textRole: "label"
                displayText: wineModel.count === 0 ? i18n("No Wine versions found. Download a build to get started.") : currentText
                QQC2.ToolTip.visible: hovered && wineModel.count === 0
                QQC2.ToolTip.text: wineModel.count === 0 ? i18n("No Wine versions found. Download a build to get started.") : ""
                onActivated: {
                    if (currentIndex >= 0 && (root.autoSaveDefaults || settingsManager.defaultWineBinary === ""))
                        settingsManager.setDefaultWineBinary(wineModel.get(currentIndex).path);
                }
            }
            QQC2.ToolButton {
                icon.name: "folder-open-symbolic"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Open Vermouth Wine folder (%1)", wineScanner.localWinePath())
                onClicked: Qt.openUrlExternally("file://" + wineScanner.localWinePath())
            }
            QQC2.ToolButton {
                icon.name: "view-refresh-symbolic"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Refresh Wine versions")
                onClicked: root.refreshWine()
            }
            QQC2.ToolButton {
                id: wineDownloadButton
                icon.name: "folder-download-symbolic"
                enabled: !wineDownloader.busy
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: wineDownloader.statusText ? wineDownloader.statusText : i18n("Download Wine build")
                onClicked: wineDownloadMenu.popup()

                QQC2.Menu {
                    id: wineDownloadMenu
                    QQC2.MenuItem {
                        text: i18n("Download latest Kron4ek Wow64 build")
                        enabled: !wineDownloader.busy
                        onTriggered: wineDownloader.downloadLatest("wow64")
                    }
                    QQC2.MenuItem {
                        text: i18n("Download latest Kron4ek regular build")
                        enabled: !wineDownloader.busy
                        onTriggered: wineDownloader.downloadLatest("regular")
                    }
                    QQC2.MenuItem {
                        text: i18n("Download latest Kron4ek TKG build")
                        enabled: !wineDownloader.busy
                        onTriggered: wineDownloader.downloadLatest("tkg")
                    }
                    QQC2.MenuItem {
                        text: i18n("Download latest Kron4ek TKG Wow64 build")
                        enabled: !wineDownloader.busy
                        onTriggered: wineDownloader.downloadLatest("tkg-wow64")
                    }
                }
            }
        }

        DownloaderProgress {
            Layout.fillWidth: true
            Kirigami.FormData.label: ""
            visible: runtimeCombo.currentValue === "wine" && wineDownloader.busy
            progress: wineDownloader.progress
            statusText: wineDownloader.statusText
        }

        RowLayout {
            Layout.fillWidth: true
            visible: runtimeCombo.currentValue === "uzdoom"
            Kirigami.FormData.label: i18n("UZDOOM AppImage:")
            QQC2.TextField {
                id: uzdoomPathField
                Layout.fillWidth: true
                placeholderText: i18n("/path/to/UZDOOM.AppImage")
                text: settingsManager.uzdoomPath
                // Only promote to the global default when none is set yet, mirroring
                // the Proton/Wine pickers. Per-game overrides are saved with the app.
                onTextChanged: if (settingsManager.uzdoomPath === "")
                    settingsManager.setUzdoomPath(text.trim())
            }
            QQC2.ToolButton {
                icon.name: "document-open-symbolic"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: i18n("Browse for UZDOOM AppImage")
                onClicked: uzdoomFileDialog.open()
            }
            QQC2.ToolButton {
                id: uzdoomDownloadButton
                icon.name: "folder-download-symbolic"
                enabled: !uzdoomDownloader.busy
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: uzdoomDownloader.statusText ? uzdoomDownloader.statusText : i18n("Download latest UZDOOM AppImage")
                onClicked: uzdoomDownloader.downloadLatest()
            }
        }

        DownloaderProgress {
            Layout.fillWidth: true
            Kirigami.FormData.label: ""
            visible: runtimeCombo.currentValue === "uzdoom" && uzdoomDownloader.busy
            progress: uzdoomDownloader.progress
            statusText: uzdoomDownloader.statusText
        }
    }

    FileDialog {
        id: uzdoomFileDialog
        title: i18n("Select UZDOOM AppImage")
        currentFolder: "file://" + protonScanner.homePath()
        nameFilters: [i18n("AppImage (*.AppImage *.appimage)"), i18n("All files (*)")]
        onAccepted: {
            var path = decodeURIComponent(selectedFile.toString().replace("file://", ""));
            uzdoomPathField.text = path;
        }
    }
}
