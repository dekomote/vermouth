import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: root
    title: i18n("Welcome to Vermouth")

    property int refresh: 0

    Component.onCompleted: defaultRuntimePicker.reset()

    function hasUmu() {
        return settingsManager.umuPath !== "";
    }

    // "", "proton", "umu", "done" or "error"
    property string autoState: ""
    property string autoError: ""
    property bool manualSetupOpen: false
    readonly property bool autoRunning: autoState === "proton" || autoState === "umu"
    readonly property bool needsSetup: {
        refresh;
        var protonReady = settingsManager.defaultProtonPath !== "" && protonScanner.isInstalled(settingsManager.defaultProtonPath);
        return !protonReady || !hasUmu();
    }

    function startAutoSetup() {
        autoError = "";
        autoState = "proton";
        protonDownloader.downloadLatest("ge");
    }

    function autoFail(message) {
        autoError = message;
        autoState = "error";
        refresh++;
    }

    Connections {
        target: protonDownloader
        function onFinished(path) {
            if (root.autoState !== "proton")
                return;
            settingsManager.setDefaultRuntimeType("proton");
            settingsManager.setDefaultProtonPath(path);
            defaultRuntimePicker.reset();
            root.refresh++;
            if (root.hasUmu()) {
                root.autoState = "done";
            } else {
                root.autoState = "umu";
                umuDownloader.downloadLatest();
            }
        }
        function onError(message) {
            if (root.autoState === "proton")
                root.autoFail(i18n("Could not download GE-Proton: %1", message));
        }
    }

    Connections {
        target: umuDownloader
        function onFinished() {
            root.refresh++;
            if (root.autoState === "umu")
                root.autoState = "done";
        }
        function onError(message) {
            root.refresh++;
            if (root.autoState === "umu")
                root.autoFail(i18n("Could not download umu-launcher: %1", message));
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pointSize: Kirigami.Theme.defaultFont.pointSize + 1
            text: i18n("Welcome to Vermouth! This screen will help you get set up for playing your games.")
            Layout.topMargin: Kirigami.Units.largeSpacing
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            visible: root.needsSetup
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: i18n("Want to get going quickly? Setting up your runtime automatically downloads the latest GE-Proton and umu-launcher and sets them as your defaults.")
            }
            QQC2.Button {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.mediumSpacing
                icon.name: "system-software-install-symbolic"
                text: root.autoRunning ? i18n("Setting up…") : (root.autoState === "error" ? i18n("Retry: Set Up Runtime Automatically") : i18n("Set Up Runtime Automatically"))
                enabled: !root.autoRunning && !protonDownloader.busy && !umuDownloader.busy
                onClicked: root.startAutoSetup()
            }
            DownloaderProgress {
                Layout.fillWidth: true
                visible: root.autoRunning
                progress: root.autoState === "proton" ? protonDownloader.progress : umuDownloader.progress
                statusText: (root.autoState === "proton" ? i18n("Step 1 of 2: GE-Proton. ") : i18n("Step 2 of 2: umu-launcher. ")) + (root.autoState === "proton" ? protonDownloader.statusText : umuDownloader.statusText)
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                visible: root.autoState === "error"
                type: Kirigami.MessageType.Error
                text: root.autoError
            }

            QQC2.Button {
                Layout.alignment: Qt.AlignHCenter
                flat: true
                icon.name: root.manualSetupOpen ? "arrow-up-symbolic" : "arrow-down-symbolic"
                text: root.manualSetupOpen ? i18n("Hide Manual Setup") : i18n("Or Set up Manually")
                onClicked: root.manualSetupOpen = !root.manualSetupOpen
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            wrapMode: Text.WordWrap
            text: i18n("You're all set! GE-Proton and umu-launcher are installed and ready to go.")
            visible: !root.needsSetup
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            visible: root.needsSetup && root.manualSetupOpen
            spacing: Kirigami.Units.smallSpacing

            RuntimePicker {
                id: defaultRuntimePicker
                Layout.fillWidth: true
                sectionLabel: i18n("Default Runtime")
                autoSaveDefaults: true
                protonWineOnly: true
            }

            Kirigami.Separator {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.largeSpacing
            }

            QQC2.Label {
                visible: {
                    root.refresh;
                    return !hasUmu();
                }
                Layout.fillWidth: true
                text: i18n("umu-launcher helps your games run better. Highly recommended.")
                Layout.topMargin: Kirigami.Units.largeSpacing
            }
            QQC2.Button {
                visible: {
                    root.refresh;
                    return !hasUmu();
                }
                icon.name: "folder-download-symbolic"
                Layout.alignment: Qt.AlignHCenter
                text: umuDownloader.busy ? i18n("Downloading umu-launcher…") : i18n("Download umu-launcher")
                enabled: !umuDownloader.busy
                onClicked: umuDownloader.downloadLatest()
            }
            DownloaderProgress {
                visible: {
                    root.refresh;
                    return !hasUmu() && umuDownloader.busy;
                }
                Layout.fillWidth: true
                progress: umuDownloader.progress
                statusText: umuDownloader.statusText
            }
        }

        Kirigami.Separator {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
        }

        QQC2.Label {
            text: i18n("Game covers make your library look amazing. Get a free SteamGridDB account and add your API key in Settings to auto-download artwork.")
            visible: settingsManager.steamGridDbApiKey === ""
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
        }
        QQC2.Button {
            icon.name: "configure"
            text: i18n("Open Settings")
            visible: settingsManager.steamGridDbApiKey === ""
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                settingsPage.load();
                applicationWindow().navigate("settings");
            }
        }

        Kirigami.Separator {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Add your games:")
            Layout.topMargin: Kirigami.Units.largeSpacing
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.mediumSpacing

            QQC2.Button {
                icon.name: "list-add-symbolic"
                text: i18n("Add a Game")
                onClicked: addDialog.openForNew()
            }
            QQC2.Button {
                icon.name: "application-x-ms-dos-executable"
                text: i18n("Install a Game")
                onClicked: installGameDialog.openDialog()
            }
            QQC2.Button {
                icon.name: "steam"
                text: i18n("Import from Steam")
                visible: steamModel.isSteamInstalled()
                onClicked: steamImportDialog.openDialog()
            }
        }

        Kirigami.Separator {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
        }

        FlatpakHostHint {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
        }

        QQC2.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing
            icon.name: "dialog-ok"
            text: i18n("Get Started")
            onClicked: applicationWindow().navigate("games")
        }
    }
}
