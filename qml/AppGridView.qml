import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GameGridView {
    id: gridView
    model: appModel

    property bool searchActive: false

    onShowHiddenChanged: appModel.showHidden = showHidden
    onSortFieldChanged: appModel.sortField = sortField
    onSortAscendingChanged: appModel.sortAscending = sortAscending
    Component.onCompleted: {
        appModel.showHidden = showHidden;
        appModel.sortField = sortField;
        appModel.sortAscending = sortAscending;
    }

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
        required property string uzdoomPath
        required property var uzdoomMods
        required property bool hidden

        readonly property bool hasPrefix: runtimeType === "proton" || runtimeType === "wine"
        opacity: cardFrame.hidden ? 0.6 : 1

        QQC2.Menu {
            id: contextMenu
            QQC2.MenuItem {
                property bool isRunning: launcher.runningExePaths.indexOf(cardFrame.exePath) >= 0
                text: isRunning ? i18n("Stop") : i18n("Launch")
                icon.name: isRunning ? "media-playback-stop-symbolic" : "media-playback-start-symbolic"
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
                icon.name: "media-playback-start-symbolic"
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
                icon.name: "edit-copy-symbolic"
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
                icon.name: "utilities-terminal-symbolic"
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
                icon.name: "system-run-symbolic"
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
                icon.name: "open-menu-symbolic"

                QQC2.MenuItem {
                    text: i18n("Create start menu entry")
                    icon.name: "open-menu-symbolic"
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
                    icon.name: "document-edit-symbolic"
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
                icon.name: "folder-open-symbolic"

                QQC2.MenuItem {
                    visible: cardFrame.runtimeType !== "steam"
                    height: visible ? implicitHeight : 0
                    text: i18n("Open install folder")
                    icon.name: "folder-open-symbolic"
                    onTriggered: {
                        var exePath = cardFrame.exePath;
                        var lastSlash = exePath.lastIndexOf('/');
                        var dir = lastSlash <= 0 ? "/" : exePath.substring(0, lastSlash);
                        Qt.openUrlExternally("file://" + dir);
                    }
                }
                QQC2.MenuItem {
                    text: i18n("Open log folder")
                    icon.name: "folder-open-symbolic"
                    onTriggered: Qt.openUrlExternally("file://" + launcher.logDir())
                }
                QQC2.MenuItem {
                    visible: cardFrame.hasPrefix
                    height: visible ? implicitHeight : 0
                    text: i18n("Open prefix folder")
                    icon.name: "folder-open-symbolic"
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
                text: cardFrame.hidden ? i18n("Unhide") : i18n("Hide")
                icon.name: cardFrame.hidden ? "view-visible-symbolic" : "view-hidden-symbolic"
                onTriggered: {
                    var app = appModel.getApp(cardFrame.index);
                    app.hidden = !app.hidden;
                    appModel.editApp(cardFrame.index, app);
                }
            }
            QQC2.MenuItem {
                text: i18n("Edit")
                icon.name: "document-edit-symbolic"
                onTriggered: addDialog.openForEdit(cardFrame.index)
            }
            QQC2.MenuItem {
                text: i18n("Delete")
                icon.name: "edit-delete-symbolic"
                onTriggered: {
                    deleteChoiceDialog.runtimeType = cardFrame.runtimeType;
                    deleteChoiceDialog.payload = cardFrame.index;
                    deleteChoiceDialog.open();
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
        readonly property string prefixPath: {
            var app = payload !== undefined ? appModel.getApp(payload) : null;
            if (!app)
                return "";
            return app.winePrefix !== "" ? app.winePrefix : app.protonPrefix;
        }
        title: i18n("Delete app and prefix?")
        standardButtons: Kirigami.Dialog.NoButton

        // Reset the confirmation field each time the dialog is shown.
        onVisibleChanged: if (visible)
            deleteConfirmField.text = ""

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: i18n("This permanently deletes the app and its prefix folder. This cannot be undone.")
            }
            QQC2.TextField {
                Layout.fillWidth: true
                readOnly: true
                visible: confirmDeleteDialog.prefixPath !== ""
                text: confirmDeleteDialog.prefixPath
            }
            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: i18n("Type DELETE to confirm.")
            }
            QQC2.TextField {
                id: deleteConfirmField
                Layout.fillWidth: true
                placeholderText: i18n("DELETE")
                onAccepted: if (text === "DELETE")
                    confirmDeleteAction.trigger()
            }
        }

        customFooterActions: [
            Kirigami.Action {
                id: confirmDeleteAction
                text: i18n("Delete")
                icon.name: "edit-delete-symbolic"
                enabled: deleteConfirmField.text === "DELETE"
                onTriggered: {
                    desktopWriter.removeShortcuts(appModel.getApp(confirmDeleteDialog.payload));
                    appModel.removeAndCleanApp(confirmDeleteDialog.payload);
                    confirmDeleteDialog.close();
                }
            },
            Kirigami.Action {
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onTriggered: confirmDeleteDialog.close()
            }
        ]
    }

    Kirigami.PromptDialog {
        id: deleteChoiceDialog
        property var payload
        property string runtimeType: ""
        title: i18n("Delete app?")
        subtitle: i18n("Choose what to delete.")
        standardButtons: Kirigami.Dialog.NoButton

        customFooterActions: [
            Kirigami.Action {
                text: i18n("Delete App")
                icon.name: "edit-delete-symbolic"
                onTriggered: {
                    confirmDeleteAppDialog.payload = deleteChoiceDialog.payload;
                    confirmDeleteAppDialog.runtimeType = deleteChoiceDialog.runtimeType;
                    deleteChoiceDialog.close();
                    confirmDeleteAppDialog.open();
                }
            },
            Kirigami.Action {
                text: i18n("Delete App and Prefix")
                icon.name: "edit-delete-symbolic"
                visible: deleteChoiceDialog.runtimeType === "proton" || deleteChoiceDialog.runtimeType === "wine"
                onTriggered: {
                    confirmDeleteDialog.payload = deleteChoiceDialog.payload;
                    deleteChoiceDialog.close();
                    confirmDeleteDialog.open();
                }
            },
            Kirigami.Action {
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onTriggered: deleteChoiceDialog.close()
            }
        ]
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
        text: gridView.searchActive ? i18n("No apps or games found") : i18n("No apps or games added yet")
        explanation: gridView.searchActive ? i18n("Try a different search term") : i18n("Click \"Add App/Game\" to get started")
        icon.name: "folder-games"
    }
}
