import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import com.dekomote.vermouth 1.0

Kirigami.ApplicationWindow {
    width: 600
    height: 700
    minimumWidth: 600
    minimumHeight: 700

    globalDrawer: Kirigami.GlobalDrawer {
        actions: [
            Kirigami.Action {
                text: i18n("Add &App/Game")
                icon.name: "list-add"
                onTriggered: addDialog.openForNew()
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
                        return i18n("%1 — %2", runner, app.exePath);
                    }
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
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

    SettingsDialog {
        id: settingsDialog
    }

    Component {
        id: aboutPage
        Kirigami.AboutPage {
            aboutData: About
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
    }
}
