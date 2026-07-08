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

    Connections {
        target: umuDownloader
        function onFinished() {
            root.refresh++;
        }
        function onError(message) {
            root.refresh++;
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

        QQC2.Label {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            wrapMode: Text.WordWrap
            text: i18n("Start by downloading a Proton runtime for Windows game support.")
        }

        RuntimePicker {
            id: defaultRuntimePicker
            Layout.fillWidth: true
            sectionLabel: i18n("Default Runtime")
            autoSaveDefaults: true
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

        Kirigami.Separator {
            Layout.topMargin: Kirigami.Units.largeSpacing
            visible: {
                root.refresh;
                return !hasUmu();
            }
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

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.mediumSpacing

            QQC2.CheckBox {
                text: i18n("Don't show me tips")
                checked: !settingsManager.showTips
                onToggled: settingsManager.setShowTips(!checked)
            }
            QQC2.Button {
                icon.name: "dialog-ok"
                text: i18n("Get Started")
                onClicked: applicationWindow().navigate("games")
            }
        }
    }
}
