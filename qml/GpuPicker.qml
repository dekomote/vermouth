import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

// Combo box for choosing a GPU. Values are PCI slots, "" and "system" are the non-GPU entries:
//   global use:   "" is "System default"
//   per-game use: "" is "Global setting" (follow the default GPU) and "system" is "System default"
// Call load() to fill it, then read `value`.
QQC2.ComboBox {
    id: root

    property bool allowInherit: false

    readonly property string value: currentIndex >= 0 ? currentValue : ""

    visible: gpuManager.available
    textRole: "text"
    valueRole: "value"

    // A saved GPU that isn't detected right now is kept as an entry, so saving doesn't drop it.
    function load(selected) {
        var list = [];
        if (allowInherit) {
            list.push({
                "text": i18n("Global setting"),
                "value": ""
            });
            list.push({
                "text": i18n("System default"),
                "value": "system"
            });
        } else {
            list.push({
                "text": i18n("System default"),
                "value": ""
            });
        }
        var known = false;
        var detected = gpuManager.gpus;
        for (var i = 0; i < detected.length; i++) {
            list.push({
                "text": detected[i].name,
                "value": detected[i].id
            });
            known = known || detected[i].id === selected;
        }
        if (selected !== "" && selected !== "system" && !known) {
            list.push({
                "text": i18n("%1 (not detected)", selected),
                "value": selected
            });
        }
        model = list;
        currentValue = selected;
    }
}
