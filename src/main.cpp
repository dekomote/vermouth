#include "appmodel.h"
#include "desktopfilewriter.h"
#include "gamepadhandler.h"
#include "iconextractor.h"
#include "launcher.h"
#include "protondownloader.h"
#include "protonscanner.h"
#include "rommclient.h"
#include "rommcovercache.h"
#include "rommfiledownloader.h"
#include "rommmodel.h"
#include "runtimetypemodel.h"
#include "settingsmanager.h"
#include "singleinstance.h"
#include "steamgriddb.h"
#include "steammodel.h"
#include "umudownloader.h"
#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickStyle>

class IconImageProvider : public QQuickImageProvider
{
public:
    IconImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
    }
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        QIcon icon = QIcon::fromTheme(id);
        if (icon.isNull())
            icon = QIcon::fromTheme(QStringLiteral("application-x-executable"));
        QPixmap pixmap = icon.pixmap(requestedSize.isValid() ? requestedSize : QSize(128, 128));
        if (size)
            *size = pixmap.size();
        return pixmap;
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("vermouth"));

    KAboutData aboutData(QStringLiteral(APP_NAME_STRING),
                         i18n("Vermouth"),
                         QStringLiteral(APP_VERSION_STRING),
                         i18n(APP_DESCRIPTION),
                         KAboutLicense::MIT,
                         i18n(APP_COPYRIGHT));
    aboutData.addAuthor(i18n(APP_AUTHOR_NAME), i18n("Lead Developer"), QStringLiteral(APP_AUTHOR_EMAIL), QStringLiteral(APP_AUTHOR_URL));
    aboutData.setBugAddress(QByteArrayLiteral(APP_BUG_ADDRESS));
    aboutData.setHomepage(QStringLiteral(APP_HOMEPAGE));
    aboutData.setDesktopFileName(QStringLiteral(APP_ID));
    KAboutData::setApplicationData(aboutData);

    qmlRegisterSingletonType(APP_ID, 1, 0, "About", [](QQmlEngine *engine, QJSEngine *) -> QJSValue {
        return engine->toScriptValue(KAboutData::applicationData());
    });

    app.setWindowIcon(QIcon(QStringLiteral(":/icons/vermouth.svg")));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    QCommandLineOption launchIdOpt(QStringLiteral("launch-id"), i18n("Launch app by its config ID"), QStringLiteral("id"));
    QCommandLineOption launchProtonOpt(QStringLiteral("launch-proton"), i18n("Launch exe with Proton"), QStringLiteral("exe"));
    QCommandLineOption launchWineOpt(QStringLiteral("launch-wine"), i18n("Launch exe with Wine"), QStringLiteral("exe"));
    QCommandLineOption protonOpt(QStringLiteral("proton"), i18n("Proton path"), QStringLiteral("path"));
    QCommandLineOption wineOpt(QStringLiteral("wine"), i18n("Wine binary path"), QStringLiteral("path"));
    QCommandLineOption prefixOpt(QStringLiteral("prefix"), i18n("Prefix path"), QStringLiteral("path"));
    QCommandLineOption launchOptsOpt(QStringLiteral("launch-options"), i18n("Launch options (use %command% as placeholder)"), QStringLiteral("options"));
    QCommandLineOption loggingOpt(QStringLiteral("enable-logging"), i18n("Enable logging to file"));
    QCommandLineOption bigPictureOpt(QStringLiteral("big-picture"), i18n("Start in Big Picture mode"));

    parser.addOption(launchIdOpt);
    parser.addOption(launchProtonOpt);
    parser.addOption(launchWineOpt);
    parser.addOption(protonOpt);
    parser.addOption(wineOpt);
    parser.addOption(prefixOpt);
    parser.addOption(launchOptsOpt);
    parser.addOption(loggingOpt);
    parser.addOption(bigPictureOpt);
    parser.addPositionalArgument(QStringLiteral("uri"), i18n("File or URI to open"), QStringLiteral("[uri...]"));
    parser.process(app);
    aboutData.processCommandLine(&parser);

    Launcher launcher;

    auto launchCli = [&](const QVariantMap &entry) -> int {
        QObject::connect(&launcher, &Launcher::processFinished, &app, &QApplication::exit);
        QObject::connect(&launcher, &Launcher::launchError, &app, [](const QString &, const QString &) {
            QApplication::exit(1);
        });
        launcher.launchEntry(entry);
        return app.exec();
    };

    // Launch by config ID - looks up entry from apps.json
    if (parser.isSet(launchIdOpt)) {
        AppModel appModel;
        QVariantMap entry = appModel.getAppById(parser.value(launchIdOpt));
        if (entry.isEmpty()) {
            qCritical() << "No app found with id:" << parser.value(launchIdOpt);
            return 1;
        }
        return launchCli(entry);
    }

    // Direct launch mode - no GUI
    if (parser.isSet(launchProtonOpt) || parser.isSet(launchWineOpt)) {
        QVariantMap entry;
        entry[QStringLiteral("launchOptions")] = parser.value(launchOptsOpt);
        entry[QStringLiteral("enableLogging")] = parser.isSet(loggingOpt);

        if (parser.isSet(launchProtonOpt)) {
            entry[QStringLiteral("runtimeType")] = QStringLiteral("proton");
            entry[QStringLiteral("protonPath")] = parser.value(protonOpt);
            entry[QStringLiteral("protonPrefix")] = parser.value(prefixOpt);
            entry[QStringLiteral("exePath")] = parser.value(launchProtonOpt);
        } else {
            entry[QStringLiteral("runtimeType")] = QStringLiteral("wine");
            entry[QStringLiteral("wineBinary")] = parser.value(wineOpt);
            entry[QStringLiteral("winePrefix")] = parser.value(prefixOpt);
            entry[QStringLiteral("exePath")] = parser.value(launchWineOpt);
        }
        return launchCli(entry);
    }

    // Check for positional args early so we can forward them if already running.
    QStringList positionalArgs = parser.positionalArguments();
    QString openExePath;
    if (!positionalArgs.isEmpty()) {
        QString arg = positionalArgs.first();
        if (arg.startsWith(QStringLiteral("file://")))
            arg = QUrl(arg).toLocalFile();
        openExePath = arg;
        AppModel tempModel;
        QVariantMap entry = tempModel.getAppByExePath(openExePath);
        if (!entry.isEmpty())
            return launchCli(entry);
    }

    SingleInstance singleInstance;
    if (!singleInstance.tryRegister()) {
        singleInstance.forwardToRunning(openExePath);
        return 0;
    }

    GamepadHandler gamepadHandler;
    AppModel appModel;
    ProtonScanner protonScanner;
    DesktopFileWriter desktopWriter;
    IconExtractor iconExtractor;
    SettingsManager settingsManager;
    ProtonDownloader protonDownloader;
    protonDownloader.setLocalProtonPath(protonScanner.localProtonPath());

    UmuDownloader umuDownloader;
    umuDownloader.setInstallPath(protonScanner.localProtonPath() + QStringLiteral("/umu"));

    SteamGridDB steamGridDb;

    SteamModel steamModel;

    RuntimeTypeModel runtimeTypeModel;

    launcher.setUmuPath(settingsManager.umuPath());
    QObject::connect(&settingsManager, &SettingsManager::umuPathChanged, [&]() {
        launcher.setUmuPath(settingsManager.umuPath());
    });

    launcher.setGlobalEnvVars(settingsManager.globalEnvVars());
    QObject::connect(&settingsManager, &SettingsManager::globalEnvVarsChanged, [&]() {
        launcher.setGlobalEnvVars(settingsManager.globalEnvVars());
    });

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        launcher.restoreHdrState();
    });
    QObject::connect(&umuDownloader, &UmuDownloader::finished, [&](const QString &binPath) {
        if (settingsManager.umuPath().isEmpty())
            settingsManager.setUmuPath(binPath);
    });

    protonScanner.setExtraProtonPaths(settingsManager.extraProtonPaths());
    protonScanner.setCustomPrefixBasePath(settingsManager.defaultPrefixDir());
    QObject::connect(&settingsManager, &SettingsManager::extraProtonPathsChanged, [&]() {
        protonScanner.setExtraProtonPaths(settingsManager.extraProtonPaths());
    });
    QObject::connect(&settingsManager, &SettingsManager::defaultPrefixDirChanged, [&]() {
        protonScanner.setCustomPrefixBasePath(settingsManager.defaultPrefixDir());
    });

    // ROMM
    RommClient rommClient;
    rommClient.setServerUrl(settingsManager.rommServerUrl());
    rommClient.setApiKey(settingsManager.rommApiKey());

    RommCoverCache rommCoverCache;
    rommCoverCache.setCacheDir(settingsManager.romCacheDir() + QStringLiteral("/covers"));
    rommCoverCache.setApiKey(settingsManager.rommApiKey());

    RommModel rommModel;
    rommModel.setClient(&rommClient);
    rommModel.setCoverCache(&rommCoverCache);

    RommFileDownloader rommFileDownloader;
    rommFileDownloader.setServerUrl(settingsManager.rommServerUrl());
    rommFileDownloader.setApiKey(settingsManager.rommApiKey());
    rommFileDownloader.setRomCacheDir(settingsManager.romCacheDir());

    launcher.setRetroarchPath(settingsManager.retroarchPath());
    launcher.setRommCoreMap(settingsManager.rommCoreMap());
    launcher.setRommGameCoreMap(settingsManager.rommGameCoreMap());

    QObject::connect(&settingsManager, &SettingsManager::rommCoreMapChanged, [&]() {
        launcher.setRommCoreMap(settingsManager.rommCoreMap());
    });
    QObject::connect(&settingsManager, &SettingsManager::rommGameCoreMapChanged, [&]() {
        launcher.setRommGameCoreMap(settingsManager.rommGameCoreMap());
    });
    QObject::connect(&settingsManager, &SettingsManager::rommServerUrlChanged, [&]() {
        rommClient.setServerUrl(settingsManager.rommServerUrl());
        rommFileDownloader.setServerUrl(settingsManager.rommServerUrl());
    });
    QObject::connect(&settingsManager, &SettingsManager::rommApiKeyChanged, [&]() {
        rommClient.setApiKey(settingsManager.rommApiKey());
        rommCoverCache.setApiKey(settingsManager.rommApiKey());
        rommFileDownloader.setApiKey(settingsManager.rommApiKey());
    });
    QObject::connect(&settingsManager, &SettingsManager::romCacheDirChanged, [&]() {
        rommCoverCache.setCacheDir(settingsManager.romCacheDir() + QStringLiteral("/covers"));
        rommFileDownloader.setRomCacheDir(settingsManager.romCacheDir());
    });
    QObject::connect(&settingsManager, &SettingsManager::retroarchPathChanged, [&]() {
        launcher.setRetroarchPath(settingsManager.retroarchPath());
    });
    QObject::connect(&launcher, &Launcher::coreAutoDetected, [&](const QString &slug, const QString &path) {
        settingsManager.setRommCore(slug, path);
    });
    QObject::connect(&rommCoverCache, &RommCoverCache::coverReady, &rommModel, &RommModel::notifyCoverCached);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new IconImageProvider);
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.rootContext()->setContextProperty(QStringLiteral("appModel"), &appModel);
    engine.rootContext()->setContextProperty(QStringLiteral("protonScanner"), &protonScanner);
    engine.rootContext()->setContextProperty(QStringLiteral("launcher"), &launcher);
    engine.rootContext()->setContextProperty(QStringLiteral("desktopWriter"), &desktopWriter);
    engine.rootContext()->setContextProperty(QStringLiteral("iconExtractor"), &iconExtractor);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsManager"), &settingsManager);
    engine.rootContext()->setContextProperty(QStringLiteral("protonDownloader"), &protonDownloader);
    engine.rootContext()->setContextProperty(QStringLiteral("umuDownloader"), &umuDownloader);
    engine.rootContext()->setContextProperty(QStringLiteral("steamGridDb"), &steamGridDb);
    engine.rootContext()->setContextProperty(QStringLiteral("steamModel"), &steamModel);
    engine.rootContext()->setContextProperty(QStringLiteral("runtimeModel"), &runtimeTypeModel);
    engine.rootContext()->setContextProperty(QStringLiteral("rommClient"), &rommClient);
    engine.rootContext()->setContextProperty(QStringLiteral("rommModel"), &rommModel);
    engine.rootContext()->setContextProperty(QStringLiteral("rommCoverCache"), &rommCoverCache);
    engine.rootContext()->setContextProperty(QStringLiteral("rommFileDownloader"), &rommFileDownloader);
    engine.rootContext()->setContextProperty(QStringLiteral("singleInstance"), &singleInstance);
    engine.rootContext()->setContextProperty(QStringLiteral("gamepadHandler"), &gamepadHandler);
    engine.rootContext()->setContextProperty(QStringLiteral("openExePath"), openExePath);
    engine.rootContext()->setContextProperty(QStringLiteral("launchBigPicture"), parser.isSet(bigPictureOpt));

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/qml/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
