import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Table of env var presets: pick one to add to `targetModel` (rows of {key, value}), or add, edit and remove presets.
Kirigami.Dialog {
    id: dialog

    property var targetModel
    property int current: -1

    title: i18n("Environment Variable Presets")
    preferredWidth: Kirigami.Units.gridUnit * 38
    preferredHeight: Kirigami.Units.gridUnit * 26
    padding: Kirigami.Units.largeSpacing

    ListModel {
        id: presetModel
    }

    ListModel {
        id: varModel
    }

    function fill(list) {
        current = -1;
        varModel.clear();
        presetModel.clear();
        for (var i = 0; i < list.length; i++)
            presetModel.append({
                "name": list[i].name,
                "description": list[i].description || "",
                "vars": JSON.stringify(list[i].vars || [])
            });
        select(presetModel.count > 0 ? 0 : -1);
    }

    function commit() {
        if (current < 0 || current >= presetModel.count)
            return;
        var vars = [];
        for (var i = 0; i < varModel.count; i++) {
            var k = varModel.get(i).key.trim();
            if (k !== "")
                vars.push(k + "=" + varModel.get(i).value.trim());
        }
        presetModel.setProperty(current, "vars", JSON.stringify(vars));
    }

    function select(index) {
        commit();
        current = index;
        varModel.clear();
        if (index < 0)
            return;
        var vars = JSON.parse(presetModel.get(index).vars);
        for (var i = 0; i < vars.length; i++) {
            var sep = vars[i].indexOf("=");
            varModel.append({
                "key": sep > 0 ? vars[i].substring(0, sep) : vars[i],
                "value": sep > 0 ? vars[i].substring(sep + 1) : ""
            });
        }
    }

    function save() {
        commit();
        var list = [];
        for (var i = 0; i < presetModel.count; i++) {
            var p = presetModel.get(i);
            list.push({
                "name": p.name,
                "description": p.description,
                "vars": JSON.parse(p.vars)
            });
        }
        settingsManager.setEnvPresets(list);
    }

    function applyCurrent() {
        if (current < 0 || !targetModel)
            return;
        commit();
        var vars = JSON.parse(presetModel.get(current).vars);
        for (var i = 0; i < vars.length; i++) {
            var sep = vars[i].indexOf("=");
            var key = (sep > 0 ? vars[i].substring(0, sep) : vars[i]).trim();
            var value = sep > 0 ? vars[i].substring(sep + 1) : "";
            var found = -1;
            for (var j = 0; j < targetModel.count; j++) {
                if (targetModel.get(j).key.trim() === key) {
                    found = j;
                    break;
                }
            }
            if (found >= 0)
                targetModel.setProperty(found, "value", value);
            else
                targetModel.append({
                    "key": key,
                    "value": value
                });
        }
    }

    onAboutToShow: fill(settingsManager.envPresets)

    customFooterActions: [
        Kirigami.Action {
            text: i18n("Add to Variables")
            icon.name: "list-add-symbolic"
            enabled: dialog.current >= 0
            onTriggered: {
                dialog.applyCurrent();
                dialog.save();
                dialog.close();
            }
        },
        Kirigami.Action {
            text: i18n("Save")
            icon.name: "document-save-symbolic"
            onTriggered: {
                dialog.save();
                dialog.close();
            }
        },
        Kirigami.Action {
            text: i18n("Cancel")
            icon.name: "dialog-cancel-symbolic"
            onTriggered: dialog.close()
        }
    ]

    RowLayout {
        spacing: Kirigami.Units.largeSpacing

        ColumnLayout {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 13
            Layout.fillHeight: true

            QQC2.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 16
                Component.onCompleted: background.visible = true

                ListView {
                    id: presetList
                    clip: true
                    model: presetModel
                    currentIndex: dialog.current
                    delegate: QQC2.ItemDelegate {
                        required property int index
                        required property string name
                        required property string description
                        width: ListView.view.width
                        highlighted: index === dialog.current
                        text: name !== "" ? name : i18n("(unnamed)")
                        onClicked: dialog.select(index)
                        QQC2.ToolTip.text: description
                        QQC2.ToolTip.visible: hovered && description !== ""
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                QQC2.Button {
                    text: i18n("New")
                    icon.name: "list-add-symbolic"
                    onClicked: {
                        dialog.commit();
                        presetModel.append({
                            "name": i18n("New preset"),
                            "description": "",
                            "vars": "[]"
                        });
                        dialog.select(presetModel.count - 1);
                    }
                }
                QQC2.ToolButton {
                    icon.name: "list-remove"
                    enabled: dialog.current >= 0
                    QQC2.ToolTip.text: i18n("Remove preset")
                    QQC2.ToolTip.visible: hovered
                    onClicked: {
                        var i = dialog.current;
                        dialog.current = -1;
                        varModel.clear();
                        presetModel.remove(i);
                        dialog.select(Math.min(i, presetModel.count - 1));
                    }
                }
                QQC2.ToolButton {
                    icon.name: "edit-undo-symbolic"
                    QQC2.ToolTip.text: i18n("Restore default presets")
                    QQC2.ToolTip.visible: hovered
                    onClicked: dialog.fill(settingsManager.defaultEnvPresets())
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            enabled: dialog.current >= 0
            spacing: Kirigami.Units.smallSpacing

            QQC2.TextField {
                Layout.fillWidth: true
                placeholderText: i18n("Preset name")
                text: dialog.current >= 0 ? presetModel.get(dialog.current).name : ""
                onTextEdited: presetModel.setProperty(dialog.current, "name", text)
            }
            QQC2.TextField {
                Layout.fillWidth: true
                placeholderText: i18n("Description")
                text: dialog.current >= 0 ? presetModel.get(dialog.current).description : ""
                onTextEdited: presetModel.setProperty(dialog.current, "description", text)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            QQC2.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    width: parent.width
                    Repeater {
                        model: varModel
                        delegate: RowLayout {
                            required property int index
                            required property string key
                            required property string value
                            Layout.fillWidth: true
                            QQC2.TextField {
                                placeholderText: i18n("KEY")
                                text: key
                                implicitWidth: Kirigami.Units.gridUnit * 9
                                onTextEdited: varModel.setProperty(index, "key", text)
                            }
                            QQC2.Label {
                                text: "="
                            }
                            QQC2.TextField {
                                placeholderText: i18n("value")
                                text: value
                                Layout.fillWidth: true
                                onTextEdited: varModel.setProperty(index, "value", text)
                            }
                            QQC2.ToolButton {
                                icon.name: "list-remove"
                                QQC2.ToolTip.text: i18n("Remove variable")
                                QQC2.ToolTip.visible: hovered
                                onClicked: varModel.remove(index)
                            }
                        }
                    }
                    QQC2.Button {
                        text: i18n("Add Variable")
                        icon.name: "list-add-symbolic"
                        flat: true
                        onClicked: varModel.append({
                            "key": "",
                            "value": ""
                        })
                    }
                }
            }
        }
    }
}
