import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    width: 600
    height: 700
    minimumWidth: 600
    minimumHeight: 700
    title: `${Application.name} (${Application.version})`

    pageStack.initialPage: Kirigami.ScrollablePage {
        id: mainPage
        title: `${Application.name}`

        actions: [
            Kirigami.Action {
                text: i18n("&Search")
                icon.name: "search"
                displayComponent: Kirigami.SearchField {
                    id: searchField
                    Layout.fillWidth: true
                    onTextChanged: appModel.setFilterString(text)
                }
            },
            Kirigami.Action {
                text: i18n("Add &App/Game")
                icon.name: "list-add"
                onTriggered: addDialog.openForNew()
            }
        ]

        AppGridView {
            anchors.fill: parent
        }
    }

    AddAppDialog {
        id: addDialog
    }

    RunExeDialog {
        id: runExeDialog
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
