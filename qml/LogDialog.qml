import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: i18n("Game Logs")
    preferredWidth: Kirigami.Units.gridUnit * 50
    preferredHeight: Kirigami.Units.gridUnit * 30
    padding: Kirigami.Units.largeSpacing
    standardButtons: Kirigami.Dialog.Close

    readonly property int maxSessionChars: 512 * 1024
    property var texts: ({})
    property var running: ({})

    function currentId() {
        return sessions.currentIndex >= 0 ? sessions.model.get(sessions.currentIndex).sessionId : "";
    }

    function showCurrent() {
        logArea.text = texts[currentId()] ?? "";
        logArea.cursorPosition = logArea.length;
    }

    Connections {
        target: launcher
        function onLogSessionStarted(id, name) {
            dialog.texts[id] = "";
            sessions.model.insert(0, {
                "sessionId": id,
                "label": name + " (" + Qt.formatTime(new Date(), "hh:mm:ss") + ")"
            });
            sessions.currentIndex = 0;
            dialog.showCurrent();
        }
        function onLogOutput(id, text) {
            var t = (dialog.texts[id] ?? "") + text;
            if (t.length > dialog.maxSessionChars)
                t = t.substring(t.length - dialog.maxSessionChars);
            dialog.texts[id] = t;
            if (id === dialog.currentId()) {
                var flick = logScroll.contentItem;
                var atEnd = flick.atYEnd;
                logArea.text = t;
                if (atEnd)
                    flick.contentY = Math.max(0, flick.contentHeight - flick.height);
            }
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            QQC2.ComboBox {
                id: sessions
                Layout.fillWidth: true
                textRole: "label"
                model: ListModel {}
                enabled: count > 0
                displayText: count > 0 ? currentText : i18n("No logged sessions yet")
                onActivated: dialog.showCurrent()
            }
            QQC2.Button {
                icon.name: "edit-copy-symbolic"
                text: i18n("Copy")
                enabled: logArea.length > 0
                onClicked: launcher.copyToClipboard(logArea.text)
            }
            QQC2.Button {
                icon.name: "edit-clear-all-symbolic"
                text: i18n("Clear")
                enabled: logArea.length > 0
                onClicked: {
                    dialog.texts[dialog.currentId()] = "";
                    dialog.showCurrent();
                }
            }
            QQC2.Button {
                icon.name: "folder-open-symbolic"
                text: i18n("Open folder")
                onClicked: Qt.openUrlExternally("file://" + launcher.logDir())
            }
        }

        QQC2.ScrollView {
            id: logScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 22

            QQC2.TextArea {
                id: logArea
                readOnly: true
                wrapMode: TextEdit.NoWrap
                font.family: "monospace"
                placeholderText: i18n("Launch a game with logging to see its output here.")
            }
        }
    }
}
