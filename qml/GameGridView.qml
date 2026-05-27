import QtQuick
import QtCore
import org.kde.kirigami as Kirigami

GridView {
    id: gridView

    property real scaleFactor: 1.0
    property string viewType: "grid"
    property bool showNames: true
    property bool active: true
    property bool lightsOut: false
    topMargin: Kirigami.Units.mediumSpacing
    bottomMargin: Kirigami.Units.mediumSpacing
    leftMargin: Kirigami.Units.mediumSpacing
    rightMargin: Kirigami.Units.mediumSpacing

    Settings {
        id: viewSettings
        category: "GridView"
        property alias scaleFactor: gridView.scaleFactor
        property alias viewType: gridView.viewType
        property alias showNames: gridView.showNames
    }

    cellWidth: {
        var base = viewType === "hero" ? 306 : viewType === "grid" ? 200 : 140;
        base *= scaleFactor;
        if (width <= 0 || count <= 0)
            return base;
        var trueWidth = (width - 2 * leftMargin);
        var cols = Math.max(1, Math.floor(trueWidth / base));
        if (count <= cols)
            return Math.floor(Math.min(trueWidth / count, base));
        return Math.floor(Math.max(base, trueWidth / cols));
    }
    cellHeight: {
        var baseH = viewType === "hero" ? 143 : viewType === "grid" ? 300 : (showNames ? 140 : 120);
        baseH *= scaleFactor;
        var baseW = viewType === "hero" ? 306 : viewType === "grid" ? 200 : 140;
        baseW *= scaleFactor;
        return Math.floor(cellWidth * (baseH / baseW));
    }

    clip: true
    focus: true
    keyNavigationEnabled: true

    onActiveFocusChanged: {
        if (!activeFocus)
            currentIndex = -1;
    }

    TapHandler {
        onTapped: {
            gridView.currentIndex = -1;
        }
    }
}
