import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import QtCore
import org.kde.kirigami as Kirigami
import com.dekomote.vermouth 1.0

Kirigami.ApplicationWindow {
    id: root
    width: 800
    height: 800
    minimumWidth: 700
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
    Kirigami.Theme.colorSet: lightsOut ? Kirigami.Theme.Complementary : Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    property double prevScaleFactor: 1
    property bool prevLightsOut: false
    property bool prevDrawerPinned: false
    property int activeTab: 0
    readonly property var tabViews: [gridView, rommView]
    function activeGridView() {
        return tabViews[activeTab] ?? gridView;
    }

    property string searchQuery: ""
    function updateSearch(text) {
        root.searchQuery = text;
        if (root.activeTab === 0) {
            appModel.setFilterString(text);
            gridView.searchActive = text !== "";
        } else {
            rommView.applySearch(text);
        }
    }
    readonly property bool searchInSidebar: !globalDrawer.modal && !root.bigPicture

    readonly property real headerHeight: Math.max(searchField.implicitHeight, rommPlatformCombo.implicitHeight, addBtn.implicitHeight) + Kirigami.Units.largeSpacing * 2

    readonly property var tabModel: [
        {
            name: i18n("Games"),
            icon: "applications-games",
            enabled: true
        },
        {
            name: i18n("RomM"),
            icon: "network-server",
            enabled: settingsManager.rommServerUrl !== ""
        }
    ]
    readonly property int enabledTabCount: tabModel.filter(t => t.enabled).length
    readonly property bool tabsInDrawer: enabledTabCount > 1 && (searchInSidebar || !settingsManager.showTabBar)

    function selectTab(index) {
        if (root.activeTab === index)
            return;
        root.activeTab = index;
        root.searchQuery = "";
        searchField.text = "";
        sidebarSearch.text = "";
        if (index === 0) {
            appModel.setFilterString("");
            gridView.searchActive = false;
        } else {
            rommView.searchText = "";
            if (settingsManager.rommServerUrl !== "" && rommView.platforms.length === 0 && !rommModel.busy)
                rommView.refresh();
        }
        Qt.callLater(() => root.activeGridView().forceActiveFocus());
    }

    Settings {
        id: windowSettings
        category: "Window"
        property int savedWidth: 800
        property int savedHeight: 800
    }

    Settings {
        id: sidebarSettings
        category: "Sidebar"
        property real width: -1
    }

    onWidthChanged: if (visibility === Window.Windowed)
        windowSettings.savedWidth = width
    onHeightChanged: if (visibility === Window.Windowed)
        windowSettings.savedHeight = height

    onLightsOutChanged: {
        if (root.lightsOut) {
            root.prevDrawerPinned = settingsManager.drawerPinned;
            if (settingsManager.drawerPinned) {
                settingsManager.setDrawerPinned(false);
                globalDrawer.close();
            }
        } else if (root.prevDrawerPinned) {
            settingsManager.setDrawerPinned(true);
            if (root.wideScreen)
                Qt.callLater(() => globalDrawer.open());
        }
    }

    background: Rectangle {
        color: root.lightsOut ? root.loBase : Kirigami.Theme.backgroundColor
    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: globalDrawer
        modal: !settingsManager.drawerPinned || !root.wideScreen
        focus: modal
        handle.visible: false
        interactiveResizeEnabled: true
        Component.onCompleted: preferredSize = sidebarSettings.width > 0 ? sidebarSettings.width : Kirigami.Units.gridUnit * 14
        onPreferredSizeChanged: sidebarSettings.width = preferredSize

        padding: 0
        topPadding: 0
        leftPadding: 0
        rightPadding: 0
        bottomPadding: 0

        Kirigami.Theme.inherit: root.lightsOut
        Kirigami.Theme.colorSet: modal ? Kirigami.Theme.Window : Kirigami.Theme.View

        header: Kirigami.AbstractApplicationHeader {
            visible: root.searchInSidebar
            preferredHeight: root.headerHeight
            Kirigami.Theme.colorSet: root.lightsOut ? Kirigami.Theme.Complementary : Kirigami.Theme.Header
            Kirigami.Theme.inherit: false
            background: Rectangle {
                color: root.lightsOut ? root.loMid : Kirigami.Theme.backgroundColor
            }

            contentItem: Kirigami.SearchField {
                id: sidebarSearch
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    leftMargin: Kirigami.Units.smallSpacing
                    rightMargin: Kirigami.Units.smallSpacing
                }
                text: root.searchQuery
                onTextChanged: root.updateSearch(text)
                onVisibleChanged: if (visible)
                    text = Qt.binding(() => root.searchQuery)
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                Kirigami.Theme.inherit: false
                color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
                placeholderTextColor: root.lightsOut ? root.loSubText : Kirigami.Theme.disabledTextColor
                background: Rectangle {
                    color: root.lightsOut ? root.loMid : Kirigami.Theme.backgroundColor
                    radius: Kirigami.Units.cornerRadius
                    border.width: 1
                    border.color: sidebarSearch.hovered || sidebarSearch.activeFocus ? Kirigami.Theme.focusColor : Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
                }
            }
        }

        topContent: [
            Repeater {
                model: root.tabsInDrawer ? root.tabModel : []
                delegate: QQC2.ItemDelegate {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    visible: modelData.enabled
                    text: modelData.name
                    icon.name: modelData.icon
                    checkable: true
                    checked: root.activeTab === index
                    onClicked: root.selectTab(index)
                }
            },
            Repeater {
                model: root.tabsInDrawer ? 1 : 0
                delegate: Kirigami.Separator {
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                }
            }
        ]

        actions: [
            Kirigami.Action {
                id: firstDrawerAction
                text: i18n("Add a Game")
                icon.name: "list-add"
                onTriggered: addDialog.openForNew()
            },
            Kirigami.Action {
                text: i18n("Run a Standalone EXE")
                icon.name: "system-run"
                onTriggered: runExeStandaloneDialog.openDialog()
            },
            Kirigami.Action {
                text: i18n("Import from Steam")
                icon.name: "steam"
                onTriggered: steamImportDialog.openDialog()
            },
            Kirigami.Action {
                text: i18n("Import GOG games")
                icon.name: "folder-games"
                onTriggered: gogImportDialog.openDialog()
            },
            Kirigami.Action {
                text: launcher.sleepInhibited ? i18n("Allow Sleep") : i18n("Prevent Sleep")
                icon.name: launcher.sleepInhibited ? "media-playback-pause" : "system-suspend-inhibited"
                onTriggered: launcher.toggleSleepInhibit()
            },
            Kirigami.Action {
                text: launcher.hdrEnabled ? i18n("Disable HDR") : i18n("Enable HDR")
                icon.name: "contrast"
                enabled: launcher.hdrSupported
                visible: launcher.hdrSupported
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
                focusPolicy: Qt.NoFocus
                checkable: true
                checked: settingsManager.drawerPinned
                // Pinning is a light-mode feature; disabled while in dark mode.
                enabled: !root.lightsOut
                flat: true
                onClicked: settingsManager.setDrawerPinned(!settingsManager.drawerPinned)
                QQC2.ToolTip.text: settingsManager.drawerPinned ? i18n("Unpin sidebar") : i18n("Pin sidebar")
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }
    }

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None
    pageStack.initialPage: Kirigami.Page {
        id: mainPage
        padding: 0

        header: ColumnLayout {
            spacing: 0

            QQC2.ToolBar {
                Layout.fillWidth: true
                Layout.preferredHeight: root.headerHeight
                topPadding: Kirigami.Units.largeSpacing
                bottomPadding: Kirigami.Units.largeSpacing
                background: Rectangle {
                    color: root.lightsOut ? root.loMid : Kirigami.Theme.backgroundColor
                }
                // AppImage hack
                palette.highlightedText: root.lightsOut ? root.loText : undefined
                palette.button: root.lightsOut ? root.loMid : undefined
                palette.buttonText: root.lightsOut ? root.loText : undefined
                palette.window: root.lightsOut ? root.loBase : undefined
                palette.windowText: root.lightsOut ? root.loText : undefined
                palette.base: root.lightsOut ? root.loBase : undefined
                palette.text: root.lightsOut ? root.loText : undefined
                palette.placeholderText: root.lightsOut ? root.loSubText : undefined
                palette.brightText: root.lightsOut ? root.loText : undefined

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.ToolButton {
                        icon.name: "application-menu"
                        focusPolicy: Qt.NoFocus
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
                        visible: !root.searchInSidebar
                        Layout.fillWidth: !root.bigPicture
                        Layout.preferredWidth: root.bigPicture ? Kirigami.Units.gridUnit * 28 : -1
                        font.pixelSize: root.bigPicture ? Math.round(Kirigami.Theme.defaultFont.pixelSize * 1.8) : Kirigami.Theme.defaultFont.pixelSize
                        text: root.searchQuery
                        onTextChanged: root.updateSearch(text)
                        onVisibleChanged: if (visible)
                            text = Qt.binding(() => root.searchQuery)
                        Kirigami.Theme.colorSet: Kirigami.Theme.View
                        Kirigami.Theme.inherit: false
                        color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
                        placeholderTextColor: root.lightsOut ? root.loSubText : Kirigami.Theme.disabledTextColor
                        background: Rectangle {
                            color: root.lightsOut ? root.loMid : Kirigami.Theme.backgroundColor
                            radius: Kirigami.Units.cornerRadius
                            border.width: 1
                            border.color: searchField.hovered || searchField.activeFocus ? Kirigami.Theme.focusColor : Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
                        }
                    }

                    Item {
                        Layout.fillWidth: root.bigPicture
                        visible: root.bigPicture
                    }

                    Item {
                        Layout.fillWidth: true
                        visible: root.searchInSidebar
                    }

                    QQC2.ToolButton {
                        id: addBtn
                        icon.name: "list-add"
                        focusPolicy: Qt.NoFocus
                        icon.color: root.lightsOut ? root.loText : "transparent"
                        visible: !root.bigPicture && root.activeTab === 0
                        onClicked: addDialog.openForNew()
                    }

                    QQC2.ComboBox {
                        id: rommPlatformCombo
                        visible: root.activeTab === 1
                        model: rommView.platforms
                        textRole: "name"
                        implicitWidth: Kirigami.Units.gridUnit * 12
                        displayText: count > 0 ? currentText : i18n("Select platform…")
                        Kirigami.Theme.colorSet: Kirigami.Theme.Button
                        Kirigami.Theme.inherit: false
                        background: Rectangle {
                            color: root.lightsOut ? root.loMid : Kirigami.Theme.backgroundColor
                            radius: Kirigami.Units.cornerRadius
                            border.width: 1
                            border.color: rommPlatformCombo.hovered || rommPlatformCombo.popup.visible ? Kirigami.Theme.focusColor : Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
                        }
                        contentItem: Text {
                            leftPadding: Kirigami.Units.smallSpacing * 2
                            rightPadding: (rommPlatformCombo.indicator ? rommPlatformCombo.indicator.width : 0) + Kirigami.Units.smallSpacing
                            text: rommPlatformCombo.displayText
                            color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        onModelChanged: {
                            var platforms = rommView.platforms;
                            if (platforms && platforms.length > 0) {
                                currentIndex = 0;
                                rommView.currentPlatformId = platforms[0].id;
                                rommModel.fetchRoms(platforms[0].id, rommView.searchText);
                            } else {
                                currentIndex = -1;
                                rommView.currentPlatformId = -1;
                            }
                        }
                        onActivated: {
                            if (currentIndex >= 0 && currentIndex < rommView.platforms.length) {
                                var plat = rommView.platforms[currentIndex];
                                rommView.currentPlatformId = plat.id;
                                rommModel.fetchRoms(plat.id, rommView.searchText);
                            }
                        }
                    }

                    QQC2.ToolButton {
                        property bool isRunning: gridView.currentIndex >= 0 && launcher.runningExePaths.indexOf(appModel.getApp(gridView.currentIndex).exePath) >= 0
                        visible: !root.bigPicture && root.activeTab === 0
                        focusPolicy: Qt.NoFocus
                        icon.name: isRunning ? "media-playback-stop" : "media-playback-start"
                        icon.color: root.lightsOut ? root.loText : "transparent"
                        enabled: gridView.currentIndex >= 0
                        onClicked: {
                            var app = appModel.getApp(gridView.currentIndex);
                            if (isRunning)
                                launcher.stopEntry(app);
                            else
                                launcher.launchEntry(app);
                        }
                    }
                }
            }

            // Tab bar
            QQC2.TabBar {
                id: mainTabBar
                Layout.fillWidth: true
                currentIndex: root.activeTab
                Kirigami.Theme.colorSet: root.lightsOut ? Kirigami.Theme.Complementary : Kirigami.Theme.Window
                Kirigami.Theme.inherit: false
                // AppImage hack
                palette.highlightedText: root.lightsOut ? root.loText : undefined
                palette.button: root.lightsOut ? root.loMid : undefined
                palette.buttonText: root.lightsOut ? root.loText : undefined
                palette.window: root.lightsOut ? root.loBase : undefined
                palette.windowText: root.lightsOut ? root.loText : undefined
                palette.base: root.lightsOut ? root.loBase : undefined
                palette.text: root.lightsOut ? root.loText : undefined
                palette.placeholderText: root.lightsOut ? root.loSubText : undefined

                visible: root.enabledTabCount > 1 && !root.tabsInDrawer

                background: Rectangle {
                    color: root.lightsOut ? root.loBase : Kirigami.Theme.backgroundColor
                }

                onCurrentIndexChanged: root.selectTab(currentIndex)

                Repeater {
                    model: root.tabModel
                    QQC2.TabButton {
                        text: modelData.name
                        enabled: modelData.enabled
                        visible: modelData.enabled
                        background: Rectangle {
                            color: root.lightsOut ? (parent.checked ? root.loBase : parent.hovered ? Qt.lighter(root.loMid, 1.15) : root.loMid) : (parent.checked ? Kirigami.Theme.backgroundColor : Qt.darker(Kirigami.Theme.backgroundColor, 1.05))
                            topLeftRadius: Kirigami.Units.cornerRadius
                            topRightRadius: Kirigami.Units.cornerRadius
                            border.width: 1
                            border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
                            Rectangle {
                                anchors {
                                    top: parent.top
                                    left: parent.left
                                    right: parent.right
                                }
                                height: 2
                                color: root.lightsOut ? root.loHighlight : Kirigami.Theme.highlightColor
                                visible: parent.parent.checked
                            }
                        }
                        contentItem: Text {
                            text: parent.text
                            color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }

        // Main content switcher
        Rectangle {
            anchors.fill: parent
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            color: root.lightsOut ? root.loBase : Kirigami.Theme.backgroundColor

            StackLayout {
                anchors.fill: parent
                currentIndex: root.activeTab

                AppGridView {
                    id: gridView
                    lightsOut: root.lightsOut
                    active: root.activeTab === 0
                }

                RommView {
                    id: rommView
                    lightsOut: root.lightsOut
                    viewType: gridView.viewType
                    scaleFactor: gridView.scaleFactor
                    showNames: gridView.showNames
                }
            }
        }

        footer: QQC2.ToolBar {
            position: QQC2.ToolBar.Footer
            topPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.largeSpacing
            Kirigami.Theme.colorSet: root.lightsOut ? Kirigami.Theme.Complementary : Kirigami.Theme.Window
            Kirigami.Theme.inherit: false
            background: Rectangle {
                color: root.lightsOut ? root.loMid : Kirigami.Theme.backgroundColor
            }
            // AppImage hack
            palette.highlightedText: root.lightsOut ? root.loText : undefined
            palette.button: root.lightsOut ? root.loMid : undefined
            palette.buttonText: root.lightsOut ? root.loText : undefined
            palette.window: root.lightsOut ? root.loBase : undefined
            palette.windowText: root.lightsOut ? root.loText : undefined
            palette.base: root.lightsOut ? root.loBase : undefined
            palette.text: root.lightsOut ? root.loText : undefined
            palette.placeholderText: root.lightsOut ? root.loSubText : undefined
            palette.brightText: root.lightsOut ? root.loText : undefined

            contentItem: RowLayout {
                QQC2.Label {
                    id: footerStatusText
                    text: ""
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                QQC2.ToolButton {
                    icon.name: "system-suspend-inhibited"
                    focusPolicy: Qt.NoFocus
                    checkable: true
                    checked: launcher.sleepInhibited
                    onClicked: launcher.toggleSleepInhibit()
                    // Unfortunately, appimage won't respect the color scheme so I have to improvise:
                    icon.color: root.lightsOut ? root.loText : (highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor)
                    QQC2.ToolTip.text: launcher.sleepInhibited ? i18n("Allow Sleep") : i18n("Prevent Sleep")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.ToolButton {
                    icon.name: "contrast"
                    focusPolicy: Qt.NoFocus
                    checkable: true
                    checked: launcher.hdrEnabled
                    enabled: launcher.hdrSupported
                    visible: launcher.hdrSupported
                    onClicked: launcher.toggleHdr()
                    icon.color: root.lightsOut ? root.loText : (highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor)
                    QQC2.ToolTip.text: launcher.hdrEnabled ? i18n("Disable HDR") : i18n("Enable HDR")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.ToolSeparator {}
                QQC2.ToolButton {
                    icon.name: "view-list-icons"
                    focusPolicy: Qt.NoFocus
                    flat: true
                    highlighted: gridView.viewType === "icon"
                    icon.color: root.lightsOut ? root.loText : (highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor)
                    onClicked: gridView.viewType = "icon"
                    QQC2.ToolTip.text: i18n("Icon view")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.ToolButton {
                    icon.name: "view-preview"
                    focusPolicy: Qt.NoFocus
                    flat: true
                    highlighted: gridView.viewType === "grid"
                    icon.color: root.lightsOut ? root.loText : (highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor)
                    onClicked: gridView.viewType = "grid"
                    QQC2.ToolTip.text: i18n("Cover art view")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.ToolButton {
                    icon.name: "image-x-generic"
                    focusPolicy: Qt.NoFocus
                    flat: true
                    highlighted: gridView.viewType === "hero"
                    icon.color: root.lightsOut ? root.loText : (highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor)
                    onClicked: gridView.viewType = "hero"
                    QQC2.ToolTip.text: i18n("Hero art view")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.ToolButton {
                    icon.name: "tag"
                    focusPolicy: Qt.NoFocus
                    flat: true
                    highlighted: gridView.showNames
                    icon.color: root.lightsOut ? root.loText : (highlighted ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor)
                    onClicked: gridView.showNames = !gridView.showNames
                    QQC2.ToolTip.text: gridView.showNames ? i18n("Hide names") : i18n("Show names")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.ToolSeparator {}
                QQC2.Button {
                    icon.name: "zoom-out"
                    focusPolicy: Qt.NoFocus
                    flat: true
                    enabled: gridView.scaleFactor > 0.8
                    onClicked: gridView.scaleFactor = Math.max(0.8, gridView.scaleFactor - 0.2)
                    icon.color: root.lightsOut ? root.loText : Kirigami.Theme.textColor
                }
                QQC2.Slider {
                    focusPolicy: Qt.NoFocus
                    from: 0.8
                    to: 1.8
                    stepSize: 0.2
                    value: gridView.scaleFactor
                    onMoved: gridView.scaleFactor = value
                    implicitWidth: Kirigami.Units.gridUnit * 6
                }
                QQC2.Button {
                    icon.name: "zoom-in"
                    focusPolicy: Qt.NoFocus
                    flat: true
                    enabled: gridView.scaleFactor < 1.8
                    onClicked: gridView.scaleFactor = Math.min(1.8, gridView.scaleFactor + 0.2)
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

    SteamImportDialog {
        id: steamImportDialog
    }

    GogImportDialog {
        id: gogImportDialog
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
        id: welcomeDialog
        title: i18n("Welcome to Vermouth")
        standardButtons: Kirigami.Dialog.NoButton
        spacing: Kirigami.Units.mediumSpacing
        customFooterActions: [
            Kirigami.Action {
                text: i18n("Don't show me tips")
                checkable: true
                checked: !settingsManager.showTips
                onTriggered: settingsManager.setShowTips(!checked)
            },
            Kirigami.Action {
                text: i18n("Close")
                icon.name: "dialog-cancel"
                onTriggered: welcomeDialog.close()
            }
        ]

        WelcomeScreen {}
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

    function updateFooterStatus() {
        if (root.activeTab === 1) {
            if (rommModel.statusText !== "")
                footerStatusText.text = rommModel.statusText;
            else if (rommModel.count > 0)
                footerStatusText.text = i18n("%1 ROMs", rommModel.count);
            else
                footerStatusText.text = "";
            return;
        }
        if (gridView.currentIndex < 0) {
            footerStatusText.text = "";
            return;
        }
        var app = appModel.getApp(gridView.currentIndex);
        var runner;
        if (app.runtimeType === "proton")
            runner = app.protonPath.split("/").pop();
        else if (app.runtimeType === "wine")
            runner = app.wineBinary;
        else
            runner = "Native";
        footerStatusText.text = i18n("%1 - %2", runner, app.exePath);
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
        root.width = windowSettings.savedWidth;
        root.height = windowSettings.savedHeight;
        if (typeof launchBigPicture !== "undefined" && launchBigPicture && !root.bigPicture)
            bigPictureAction.trigger();
        if (typeof openExePath !== "undefined" && openExePath !== "")
            root.openExe(openExePath);
        else
            maybeShowWelcome();
    }

    function maybeShowWelcome() {
        var firstRun = !settingsManager.firstRunComplete;
        if (!firstRun)
            return;
        settingsManager.setFirstRunComplete(true);
        if (root.bigPicture)
            return;
        if (!settingsManager.showTips)
            return;
        if (appModel.count > 0)
            return;
        welcomeDialog.open();
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
                root.activeGridView().forceActiveFocus();
            } else {
                globalDrawer.open();
            }
        }

        function onYPressed() {
            if (globalDrawer.drawerOpen)
                globalDrawer.close();
            searchField.forceActiveFocus();
            gridView.currentIndex = -1;
            Qt.inputMethod.show();
        }

        function onBPressed() {
            if (rommPlatformCombo.popup.visible) {
                rommPlatformCombo.popup.close();
                root.activeGridView().forceActiveFocus();
            } else if (globalDrawer.drawerOpen) {
                globalDrawer.close();
                root.activeGridView().forceActiveFocus();
            } else {
                gridView.currentIndex = -1;
                root.activeGridView().forceActiveFocus();
            }
        }

        function onAPressed() {
            gamepadHandler.sendKey(Qt.Key_Return);
        }

        function onGuidePressed() {
            if (settingsManager.gamepadFullscreenButton === "guide")
                bigPictureAction.trigger();
        }

        function onSelectL2Pressed() {
            if (settingsManager.gamepadFullscreenButton === "selectl2")
                bigPictureAction.trigger();
        }

        function onL3r3Pressed() {
            if (settingsManager.gamepadFullscreenButton === "l3r3")
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

        function onL1Pressed() {
            if (root.enabledTabCount > 1)
                root.selectTab(Math.max(0, root.activeTab - 1));
        }

        function onR1Pressed() {
            if (root.enabledTabCount > 1)
                root.selectTab(Math.min(root.tabModel.length - 1, root.activeTab + 1));
        }

        function onR2Pressed() {
            if (root.activeTab === 1 && rommPlatformCombo.visible) {
                if (rommPlatformCombo.popup.visible) {
                    rommPlatformCombo.popup.close();
                    rommPlatformCombo.popup.contentItem.currentIndex = 0;
                    root.activeGridView().forceActiveFocus();
                } else {
                    rommPlatformCombo.popup.open();
                    Qt.callLater(function () {
                        rommPlatformCombo.popup.contentItem.currentIndex = 0;
                        rommPlatformCombo.popup.contentItem.forceActiveFocus();
                    });
                }
            }
        }
    }

    Connections {
        target: globalDrawer
        function onDrawerOpenChanged() {
            if (globalDrawer.drawerOpen && globalDrawer.modal) {
                drawerFocusTimer.start();
            } else if (!globalDrawer.drawerOpen) {
                drawerFocusTimer.stop();
                root.activeGridView().forceActiveFocus();
            }
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
            showPassiveNotification(i18n("Error launching: %1", error), 6000);
        }
        function onPrefixNotReady(name) {
            prefixNotReadyDialog.appName = name;
            prefixNotReadyDialog.open();
        }
    }

    Connections {
        target: gridView
        function onCurrentIndexChanged() {
            root.updateFooterStatus();
        }
    }

    Connections {
        target: rommModel
        function onCountChanged() {
            if (root.activeTab === 1)
                root.updateFooterStatus();
        }
        function onStatusTextChanged() {
            if (root.activeTab === 1)
                root.updateFooterStatus();
        }
    }

    onActiveTabChanged: root.updateFooterStatus()

    Connections {
        target: steamGridDb
        function onAutoDownloadProgress(name) {
            footerStatusText.text = i18n("Auto downloading: %1", name);
        }
        function onAutoDownloadFinished() {
            root.updateFooterStatus();
        }
    }

    CorePickerDialog {
        id: mainCorePicker
    }
}
