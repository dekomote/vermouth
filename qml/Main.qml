import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
import com.dekomote.vermouth 1.0

Kirigami.ApplicationWindow {
    id: root
    width: 800
    height: 800
    minimumWidth: settingsManager.drawerPinned ? 900 : 700
    minimumHeight: 800
    visibility: settingsManager.bigPicture ? Window.FullScreen : Window.Windowed

    // Lights Out computed colors
    readonly property bool lightsOut: settingsManager.lightsOut
    readonly property bool bigPicture: settingsManager.bigPicture
    readonly property color loBase: Qt.color(settingsManager.lightsOutColor)
    readonly property color loDark: Qt.darker(loBase, 1.5)
    readonly property color loDarkest: Qt.darker(loBase, 2)
    readonly property color loMid: Qt.darker(loBase, 1.2)
    readonly property color loHighlight: Qt.lighter(loBase, 1.8)
    readonly property color loText: "#ffffff"
    readonly property color loSubText: Qt.rgba(1, 1, 1, 0.6)
    readonly property color loAltBg: Qt.darker(loBase, 1.3)
    property double prevScaleFactor: 1
    property bool prevLightsOut: false

    background: Rectangle {
        color: root.lightsOut ? "transparent" : Kirigami.Theme.backgroundColor
    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: globalDrawer
        modal: !settingsManager.drawerPinned
        focus: modal
        Kirigami.Theme.colorSet: root.lightsOut ? Kirigami.Theme.Complementary : Kirigami.Theme.Window
        handle.visible: false
        background: Rectangle {
            color: root.lightsOut ? root.loBase : Kirigami.Theme.backgroundColor
        }

        actions: [
            Kirigami.Action {
                id: firstDrawerAction
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
                text: root.lightsOut ? i18n("Lights On") : i18n("Lights Out")
                icon.name: root.lightsOut ? "weather-clear" : "weather-clear-night"
                onTriggered: settingsManager.setLightsOut(!root.lightsOut)
            },
            Kirigami.Action {
                id: bigPictureAction
                text: root.bigPicture ? i18n("Exit Big Picture") : i18n("Big Picture")
                icon.name: root.bigPicture ? "view-restore" : "view-fullscreen"
                shortcut: "F11"
                onTriggered: {
                    if (!root.bigPicture) {
                        root.prevLightsOut = root.lightsOut;
                        root.prevScaleFactor = gridView.scaleFactor;
                        root.visibility = Window.FullScreen;
                        settingsManager.setLightsOut(true);
                        gridView.scaleFactor = 1.5;
                    } else {
                        root.visibility = Window.Windowed;
                        settingsManager.setLightsOut(root.prevLightsOut);
                        gridView.scaleFactor = root.prevScaleFactor;
                    }
                    settingsManager.setBigPicture(!root.bigPicture);
                }
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

        footer: RowLayout {
            Item {
                Layout.fillWidth: true
            }
            QQC2.ToolButton {
                icon.name: "pin"
                checkable: true
                checked: settingsManager.drawerPinned
                flat: true
                onClicked: settingsManager.setDrawerPinned(!settingsManager.drawerPinned)
                QQC2.ToolTip.text: settingsManager.drawerPinned ? i18n("Unpin sidebar") : i18n("Pin sidebar")
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }
    }

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None
    pageStack.initialPage: Kirigami.ScrollablePage {
        id: mainPage

        background: Rectangle {
            color: root.lightsOut ? root.loBase : Kirigami.Theme.backgroundColor
        }

        header: QQC2.ToolBar {
            Kirigami.Theme.colorSet: root.lightsOut ? Kirigami.Theme.Complementary : Kirigami.Theme.Window
            topPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.largeSpacing

            background: Rectangle {
                color: root.lightsOut ? root.loMid : Kirigami.Theme.backgroundColor
            }

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.ToolButton {
                    icon.name: "application-menu"
                    visible: globalDrawer.modal
                    onClicked: globalDrawer.open()
                    icon.color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
                }

                Item {
                    Layout.fillWidth: root.bigPicture
                    visible: root.bigPicture
                }

                Kirigami.SearchField {
                    id: searchField
                    Layout.fillWidth: !root.bigPicture
                    Layout.preferredWidth: root.bigPicture ? Kirigami.Units.gridUnit * 28 : -1
                    font.pixelSize: root.bigPicture ? Math.round(Kirigami.Theme.defaultFont.pixelSize * 1.8) : Kirigami.Theme.defaultFont.pixelSize
                    onTextChanged: appModel.setFilterString(text)
                    color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
                    background: Rectangle {
                        color: root.lightsOut ? "transparent" : Kirigami.Theme.backgroundColor
                        border.color: root.lightsOut ? root.loHighlight : Kirigami.Theme.disabledTextColor
                        radius: 4
                    }
                }

                Item {
                    Layout.fillWidth: root.bigPicture
                    visible: root.bigPicture
                }
                QQC2.ToolButton {
                    text: i18n("Add &App/Game")
                    icon.name: "list-add"
                    icon.color: root.lightsOut ? root.loText : "transparent"
                    onClicked: addDialog.openForNew()
                    visible: !root.bigPicture
                }
                QQC2.ToolButton {
                    property bool isRunning: gridView.selectedIndex >= 0 && launcher.runningExePaths.indexOf(appModel.getApp(gridView.selectedIndex).exePath) >= 0
                    visible: !root.bigPicture
                    icon.name: isRunning ? "media-playback-stop" : "media-playback-start"
                    icon.color: root.lightsOut ? root.loText : "transparent"
                    enabled: gridView.selectedIndex >= 0
                    onClicked: {
                        var app = appModel.getApp(gridView.selectedIndex);
                        if (isRunning)
                            launcher.stopEntry(app);
                        else
                            launcher.launchEntry(app);
                    }
                }
            }
        }

        AppGridView {
            id: gridView
            anchors.fill: parent
            lightsOut: root.lightsOut
        }

        footer: QQC2.ToolBar {
            Kirigami.Theme.colorSet: root.lightsOut ? Kirigami.Theme.Complementary : Kirigami.Theme.Window
            position: QQC2.ToolBar.Footer
            topPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.largeSpacing

            background: Rectangle {
                color: root.lightsOut ? root.loMid : Kirigami.Theme.backgroundColor
            }

            contentItem: RowLayout {
                QQC2.Label {
                    text: {
                        if (gridView.selectedIndex < 0)
                            return "";
                        var app = appModel.getApp(gridView.selectedIndex);
                        var runner = app.runtimeType === "proton" ? app.protonPath.split("/").pop() : app.wineBinary;
                        return i18n("%1 - %2", runner, app.exePath);
                    }
                    color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
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
                    icon.color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
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
                    icon.color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
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

    Kirigami.PromptDialog {
        id: openExeChoiceDialog
        property string exePath: ""
        title: i18n("Open Executable")
        subtitle: exePath
        standardButtons: Kirigami.Dialog.NoButton
        customFooterActions: [
            Kirigami.Action {
                text: i18n("Run Standalone")
                icon.name: "media-playback-start"
                onTriggered: {
                    openExeChoiceDialog.close();
                    runExeStandaloneDialog.openDialog(openExeChoiceDialog.exePath);
                }
            },
            Kirigami.Action {
                text: i18n("Add to Library")
                icon.name: "list-add"
                onTriggered: {
                    openExeChoiceDialog.close();
                    addDialog.openForNewWithExe(openExeChoiceDialog.exePath);
                }
            }
        ]
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

    function focusFirstDrawerItem() {
        function findFocusable(item) {
            if (!item)
                return null;
            if (item.activeFocusOnTab && item.visible && item.enabled)
                return item;
            var kids = item.children;
            for (var i = 0; i < kids.length; i++) {
                var found = findFocusable(kids[i]);
                if (found)
                    return found;
            }
            return null;
        }
        var target = findFocusable(globalDrawer.contentItem);
        if (target)
            target.forceActiveFocus();
    }

    function openExe(path) {
        var existing = appModel.getAppByExePath(path);
        if (existing && existing.exePath !== undefined) {
            launcher.launchEntry(existing);
        } else {
            openExeChoiceDialog.exePath = path;
            openExeChoiceDialog.open();
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
                root.openExe(path);
        }
    }

    Component.onCompleted: {
        if (typeof launchBigPicture !== "undefined" && launchBigPicture && !root.bigPicture)
            bigPictureAction.trigger();
        if (typeof openExePath !== "undefined" && openExePath !== "")
            root.openExe(openExePath);
    }

    Connections {
        target: singleInstance
        function onOpenExeRequested(path) {
            root.openExe(path);
        }
        function onRaiseRequested() {
            root.raise();
            root.requestActivate();
        }
    }

    Connections {
        target: gamepadHandler

        function onSelectPressed() {
            if (!globalDrawer.modal)
                return;
            if (globalDrawer.drawerOpen) {
                globalDrawer.close();
                gridView.forceActiveFocus();
            } else {
                globalDrawer.open();
            }
        }

        function onYPressed() {
            if (globalDrawer.drawerOpen)
                globalDrawer.close();
            searchField.forceActiveFocus();
            gridView.selectedIndex = -1;
            gridView.currentIndex = -1;
            Qt.inputMethod.show();
        }

        function onBPressed() {
            if (globalDrawer.drawerOpen) {
                globalDrawer.close();
                gridView.forceActiveFocus();
            } else {
                gridView.currentIndex = -1;
                gridView.forceActiveFocus();
            }
        }

        function onAPressed() {
            gamepadHandler.sendKey(Qt.Key_Return);
        }

        function onStartPressed() {
            bigPictureAction.trigger();
        }

        function onDpadUp() {
            gamepadHandler.sendKey(Qt.Key_Up);
        }

        function onDpadDown() {
            gamepadHandler.sendKey(Qt.Key_Down);
        }

        function onDpadLeft() {
            gamepadHandler.sendKey(Qt.Key_Left);
        }

        function onDpadRight() {
            gamepadHandler.sendKey(Qt.Key_Right);
        }
    }

    Connections {
        target: globalDrawer
        function onDrawerOpenChanged() {
            if (globalDrawer.drawerOpen && globalDrawer.modal)
                drawerFocusTimer.start();
            else
                drawerFocusTimer.stop();
        }
    }

    Timer {
        id: drawerFocusTimer
        interval: 50
        onTriggered: root.focusFirstDrawerItem()
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
