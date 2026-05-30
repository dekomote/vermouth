import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GameGridView {
    id: gridView
    model: appModel

    Connections {
        target: launcher
        function onRomCoreMissing(platformSlug, rom) {
            if (gridView.active) {
                mainCorePicker.platformSlug = platformSlug;
                mainCorePicker.pendingRom = rom;
                mainCorePicker.appIndex = -1;
                mainCorePicker.open();
            }
        }
    }

    Shortcut {
        sequence: "Return"
        enabled: gridView.active && gridView.currentIndex >= 0
        onActivated: {
            var app = appModel.getApp(gridView.currentIndex);
            launcher.launchEntry(app);
        }
    }
    Shortcut {
        sequence: "Delete"
        enabled: gridView.active && gridView.currentIndex >= 0
        onActivated: {
            var app = appModel.getApp(gridView.currentIndex);
            confirmDeleteAppDialog.runtimeType = app.runtimeType;
            confirmDeleteAppDialog.payload = gridView.currentIndex;
            confirmDeleteAppDialog.open();
        }
    }
    Shortcut {
        sequence: "Shift+Delete"
        enabled: gridView.active && gridView.currentIndex >= 0 && appModel.getApp(gridView.currentIndex).runtimeType !== "native"
        onActivated: {
            confirmDeleteDialog.payload = gridView.currentIndex;
            confirmDeleteDialog.open();
        }
    }

    delegate: GameCardFrame {
        id: cardFrame
        gv: gridView
        badgeType: runtimeType
        displayName: name
        artSource: {
            var v = gridView.viewType;
            if (v === "hero")
                return heroPath !== "" ? "file://" + heroPath : "";
            if (v === "grid")
                return gridPath !== "" ? "file://" + gridPath : "";
            return iconPath !== "" ? (iconPath.startsWith("/") ? "file://" + iconPath : "image://icon/" + iconPath) : "";
        }
        iconFallback: iconPath !== "" ? (iconPath.startsWith("/") ? "file://" + iconPath : "image://icon/" + iconPath) : ""
        heroLogo: logoPath !== "" ? "file://" + logoPath : ""

        required property string appId
        required property string name
        required property string exePath
        required property string runtimeType
        required property string protonPath
        required property string protonPrefix
        required property string wineBinary
        required property string winePrefix
        required property string iconPath
        required property string gridPath
        required property string heroPath
        required property string logoPath
        required property string launchOptions
        required property bool enableLogging
        required property int steamAppId
        required property string platformSlug
        required property string customCorePath

        readonly property bool hasPrefix: runtimeType === "proton" || runtimeType === "wine"

        QQC2.Menu {
            id: contextMenu
            QQC2.MenuItem {
                property bool isRunning: launcher.runningExePaths.indexOf(cardFrame.exePath) >= 0
                text: isRunning ? i18n("Stop") : i18n("Launch")
                icon.name: isRunning ? "media-playback-stop" : "media-playback-start"
                onTriggered: {
                    var app = appModel.getApp(cardFrame.index);
                    if (isRunning) {
                        launcher.stopEntry(app);
                    } else {
                        cardFrame.playLaunchAnimation();
                        launcher.launchEntry(app);
                    }
                }
            }
            QQC2.MenuItem {
                visible: cardFrame.runtimeType === "steam" && cardFrame.steamAppId > 0
                height: visible ? implicitHeight : 0
                text: i18n("View in Steam")
                icon.name: "steam"
                onTriggered: Qt.openUrlExternally("steam://nav/games/details/" + cardFrame.steamAppId)
            }
            QQC2.MenuItem {
                visible: cardFrame.runtimeType === "retroarch"
                height: visible ? implicitHeight : 0
                text: i18n("Change Core…")
                icon.name: "media-record"
                onTriggered: {
                    mainCorePicker.platformSlug = cardFrame.platformSlug;
                    mainCorePicker.pendingRom = null;
                    mainCorePicker.appIndex = cardFrame.index;
                    mainCorePicker.launchAfterPick = false;
                    mainCorePicker.open();
                }
            }
            QQC2.MenuItem {
                visible: cardFrame.runtimeType === "retroarch"
                height: visible ? implicitHeight : 0
                text: i18n("Copy Launch Command")
                icon.name: "edit-copy"
                onTriggered: {
                    var app = appModel.getApp(cardFrame.index);
                    var rom = {
                        "localRomPath": app.exePath,
                        "platformSlug": app.platformSlug,
                        "name": app.name,
                        "romId": 0,
                        "customCorePath": app.customCorePath
                    };
                    var cmd = launcher.buildRomLaunchCommand(rom);
                    if (cmd !== "")
                        launcher.copyToClipboard(cmd);
                }
            }
            QQC2.MenuItem {
                visible: cardFrame.runtimeType !== "steam"
                height: visible ? implicitHeight : 0
                text: i18n("Launch with logging")
                icon.name: "text-x-log"
                onTriggered: {
                    cardFrame.playLaunchAnimation();
                    var app = appModel.getApp(cardFrame.index);
                    app.enableLogging = true;
                    launcher.launchEntry(app);
                    Qt.openUrlExternally("file://" + launcher.logDir());
                }
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                visible: cardFrame.hasPrefix
                height: visible ? implicitHeight : 0
                text: i18n("Run another EXE in this prefix")
                icon.name: "system-run"
                onTriggered: {
                    runExeDialog.appIndex = cardFrame.index;
                    runExeDialog.open();
                }
            }
            QQC2.MenuSeparator {
                visible: cardFrame.hasPrefix
                height: visible ? implicitHeight : 0
            }
            QQC2.Menu {
                title: i18n("Create shortcut")
                icon.name: "application-menu"

                QQC2.MenuItem {
                    text: i18n("Create start menu entry")
                    icon.name: "application-menu"
                    onTriggered: {
                        var app = appModel.getApp(cardFrame.index);
                        desktopWriter.createStartMenuEntry(app);
                    }
                }
                QQC2.MenuItem {
                    text: i18n("Create desktop shortcut")
                    icon.name: "user-desktop"
                    onTriggered: {
                        var app = appModel.getApp(cardFrame.index);
                        desktopWriter.createDesktopShortcut(app);
                    }
                }
            }
            QQC2.Menu {
                enabled: cardFrame.hasPrefix
                title: i18n("&Wine Utilities")
                icon.name: "wine-symbolic"

                QQC2.MenuItem {
                    text: i18n("Run Winecfg")
                    icon.name: "preferences-system"
                    onTriggered: {
                        var app = appModel.getApp(cardFrame.index);
                        launcher.runWinecfg(app);
                    }
                }
                QQC2.MenuItem {
                    text: i18n("Run Regedit")
                    icon.name: "document-edit"
                    onTriggered: {
                        var app = appModel.getApp(cardFrame.index);
                        launcher.runRegedit(app);
                    }
                }
                QQC2.MenuItem {
                    text: i18n("Run Winetricks")
                    icon.name: "tools-symbolic"
                    onTriggered: {
                        if (!launcher.isWinetricksAvailable()) {
                            winetricksNotFoundDialog.open();
                            return;
                        }
                        var app = appModel.getApp(cardFrame.index);
                        launcher.runWinetricks(app);
                    }
                }
            }
            QQC2.MenuSeparator {}
            QQC2.Menu {
                title: i18n("Folders")
                icon.name: "folder-open"

                QQC2.MenuItem {
                    visible: cardFrame.runtimeType !== "steam"
                    height: visible ? implicitHeight : 0
                    text: i18n("Open install folder")
                    icon.name: "folder-open"
                    onTriggered: {
                        var exePath = cardFrame.exePath;
                        var lastSlash = exePath.lastIndexOf('/');
                        var dir = lastSlash > 0 ? exePath.substring(0, lastSlash) : exePath;
                        Qt.openUrlExternally("file://" + dir);
                    }
                }
                QQC2.MenuItem {
                    text: i18n("Open log folder")
                    icon.name: "folder-open"
                    onTriggered: Qt.openUrlExternally("file://" + launcher.logDir())
                }
                QQC2.MenuItem {
                    visible: cardFrame.hasPrefix
                    height: visible ? implicitHeight : 0
                    text: i18n("Open prefix folder")
                    icon.name: "folder-open"
                    onTriggered: {
                        var prefix = cardFrame.runtimeType === "proton" ? cardFrame.protonPrefix : cardFrame.winePrefix;
                        if (prefix !== "")
                            Qt.openUrlExternally("file://" + prefix);
                    }
                    enabled: (cardFrame.runtimeType === "proton" ? cardFrame.protonPrefix : cardFrame.winePrefix) !== ""
                }
            }
            QQC2.MenuSeparator {}
            QQC2.MenuItem {
                text: i18n("Edit")
                icon.name: "document-edit"
                onTriggered: addDialog.openForEdit(cardFrame.index)
            }
            QQC2.MenuItem {
                text: i18n("Remove")
                icon.name: "edit-delete"
                onTriggered: {
                    confirmDeleteAppDialog.runtimeType = cardFrame.runtimeType;
                    confirmDeleteAppDialog.payload = cardFrame.index;
                    confirmDeleteAppDialog.open();
                }
            }
            QQC2.MenuItem {
                visible: cardFrame.hasPrefix
                height: visible ? implicitHeight : 0
                text: i18n("Remove and Delete Prefix")
                icon.name: "edit-delete"
                onTriggered: {
                    confirmDeleteDialog.payload = cardFrame.index;
                    confirmDeleteDialog.open();
                }
            }
        }

        onLaunched: {
            var app = appModel.getApp(cardFrame.index);
            launcher.launchEntry(app);
        }
        onContextMenuRequested: {
            contextMenu.popup();
        }
    }

    Kirigami.PromptDialog {
        id: confirmDeleteDialog
        property var payload
        title: i18n("Delete both app and prefix?")
        subtitle: i18n("This will delete both the app and the prefix?")
        onAccepted: {
            desktopWriter.removeShortcuts(appModel.getApp(payload));
            appModel.removeAndCleanApp(payload);
        }
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    }

    Kirigami.PromptDialog {
        id: confirmDeleteAppDialog
        property var payload
        property string runtimeType: ""
        title: i18n("Delete the app?")
        subtitle: runtimeType === "native" || runtimeType === "retroarch" || runtimeType === "steam" ? i18n("This will delete the app from the library.") : i18n("This will delete the app but preserve the prefix folder.")
        onAccepted: {
            desktopWriter.removeShortcuts(appModel.getApp(payload));
            appModel.removeApp(payload);
        }
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    }

    Kirigami.PromptDialog {
        id: winetricksNotFoundDialog
        title: i18n("Winetricks not found")
        subtitle: i18n("Winetricks is not installed on your system. Please install it using your package manager.")
        standardButtons: Kirigami.Dialog.Ok
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: gridView.count === 0
        text: i18n("No apps or games added yet")
        explanation: i18n("Click \"Add App/Game\" to get started")
        icon.name: "games-config-custom"
    }
}
