import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    width: 600
    height: 700
    minimumWidth: 600
    minimumHeight: 700
    visible: true
    title: "Vermouth"

    pageStack.initialPage: Kirigami.ScrollablePage {
        id: mainPage
        supportsRefreshing: false

        header: QQC2.ToolBar {
            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Image {
                    source: "qrc:/icons/vermouth.svg"
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    fillMode: Image.PreserveAspectFit
                }

                Kirigami.SearchField {
                    id: searchField
                    Layout.fillWidth: true
                    onTextChanged: appModel.setFilterString(text)
                }

                QQC2.Button {
                    text: "Add App/Game"
                    icon.name: "list-add"
                    onClicked: addDialog.openForNew()
                    highlighted: true
                }
            }
        }

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
            showPassiveNotification("Launched: " + name, 3000)
        }
        function onLaunchError(name, error) {
            showPassiveNotification("Error: " + error, 5000)
        }
    }
}
