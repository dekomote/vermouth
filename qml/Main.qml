import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import com.dekomote.vermouth 1.0

Kirigami.ApplicationWindow {
    width: 700
    height: 800
    minimumWidth: 700
    minimumHeight: 800

    globalDrawer: Kirigami.GlobalDrawer {
        actions: [
            Kirigami.Action {
                text: i18n("Add &App/Game")
                icon.name: "list-add"
                onTriggered: addDialog.openForNew()
            },
            Kirigami.Action {
                text: i18n("Run &Standalone EXE")
                icon.name: "system-run"
                onTriggered: runExeStandaloneDialog.openDialog()
            },
            Kirigami.Action {
                text: launcher.sleepInhibited ? i18n("Allow Sleep") : i18n("Prevent Sleep")
                icon.name: launcher.sleepInhibited ? "media-playback-pause" : "system-suspend-inhibited"
                checkable: true
                checked: launcher.sleepInhibited
                onTriggered: launcher.toggleSleepInhibit()
            },
            Kirigami.Action {
                text: launcher.hdrEnabled ? i18n("Disable HDR") : i18n("Enable HDR")
                icon.name: "contrast"
                checkable: true
                checked: launcher.hdrEnabled
                enabled: launcher.hdrSupported
                onTriggered: launcher.toggleHdr()
            },
            Kirigami.Action {
                text: i18n("&Settings")
                icon.name: "configure"
                onTriggered: settingsDialog.openDialog()
            },
            Kirigami.Action {
                text: i18n("&About Vermouth")
                icon.name: "help-about"
                onTriggered: pageStack.pushDialogLayer(aboutPage)
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit-symbolic"
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        ]
    }

    pageStack.initialPage: Kirigami.ScrollablePage {
        id: mainPage

        titleDelegate: RowLayout {
            Layout.fillWidth: true
            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                onTextChanged: appModel.setFilterString(text)
            }
            QQC2.Button {
                text: i18n("Add &App/Game")
                icon.name: "list-add"
                onClicked: addDialog.openForNew()
            }
            QQC2.Button {
                icon.name: "media-playback-start"
                enabled: gridView.selectedIndex >= 0
                onClicked: {
                    var app = appModel.getApp(gridView.selectedIndex);
                    launcher.launchEntry(app);
                }
            }
        }

        AppGridView {
            id: gridView
            anchors.fill: parent
        }

        footer: QQC2.ToolBar {
            position: QQC2.ToolBar.Footer
            contentItem: RowLayout {
                QQC2.Label {
                    text: {
                        if (gridView.selectedIndex < 0)
                            return "";
                        var app = appModel.getApp(gridView.selectedIndex);
                        var runner = app.runtimeType === "proton" ? app.protonPath.split("/").pop() : app.wineBinary;
                        return i18n("%1 - %2", runner, app.exePath);
                    }
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                QQC2.ToolButton {
                    icon.name: "system-suspend-inhibited"
                    checkable: true
                    checked: launcher.sleepInhibited
                    onClicked: launcher.toggleSleepInhibit()
                    QQC2.ToolTip.text: launcher.sleepInhibited ? i18n("Allow Sleep") : i18n("Prevent Sleep")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.ToolButton {
                    icon.name: "contrast"
                    checkable: true
                    checked: launcher.hdrEnabled
                    enabled: launcher.hdrSupported
                    onClicked: launcher.toggleHdr()
                    QQC2.ToolTip.text: launcher.hdrEnabled ? i18n("Disable HDR") : i18n("Enable HDR")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.Button {
                    icon.name: "zoom-out"
                    flat: true
                    enabled: gridView.scaleFactor > 0.5
                    onClicked: gridView.scaleFactor = Math.max(0.5, gridView.scaleFactor - 0.25)
                }
                QQC2.Slider {
                    from: 0.5
                    to: 2.0
                    stepSize: 0.25
                    value: gridView.scaleFactor
                    onMoved: gridView.scaleFactor = value
                    implicitWidth: Kirigami.Units.gridUnit * 6
                }
                QQC2.Button {
                    icon.name: "zoom-in"
                    flat: true
                    enabled: gridView.scaleFactor < 2.0
                    onClicked: gridView.scaleFactor = Math.min(2.0, gridView.scaleFactor + 0.25)
                }
            }
        }
    }

    AddAppDialog {
        id: addDialog
    }

    RunExeDialog {
        id: runExeDialog
    }

    RunExeStandaloneDialog {
        id: runExeStandaloneDialog
    }

    SettingsDialog {
        id: settingsDialog
    }

    Kirigami.PromptDialog {
        id: prefixNotReadyDialog
        property string appName
        title: i18n("Prefix not ready")
        subtitle: i18n("The prefix for \"%1\" has not been created yet. Please launch the game at least once first.", appName)
        standardButtons: Kirigami.Dialog.Ok
    }

    Component {
        id: aboutPage
        Kirigami.AboutPage {
            aboutData: About
        }
    }

    DropArea {
        anchors.fill: parent
        onDropped: function (drop) {
            var path = "";
            if (drop.hasUrls) {
                path = decodeURIComponent(drop.urls[0].toString().replace("file://", ""));
            } else if (drop.hasText) {
                path = decodeURIComponent(drop.text.trim().replace("file://", ""));
            }
            if (path !== "")
                runExeStandaloneDialog.openDialog(path);
        }
    }

    Component.onCompleted: {
        if (typeof openExePath !== "undefined" && openExePath !== "") {
            runExeStandaloneDialog.openDialog(openExePath);
        }
    }

    Connections {
        target: launcher
        function onLaunched(name) {
            showPassiveNotification(i18n("Launched: %1", name), 3000);
        }
        function onLaunchError(name, error) {
            showPassiveNotification(i18n("Error: %1", error), 5000);
        }
        function onPrefixNotReady(name) {
            prefixNotReadyDialog.appName = name;
            prefixNotReadyDialog.open();
        }
    }
}
