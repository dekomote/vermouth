#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QQuickStyle>
#include <QIcon>
#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>
#include "appmodel.h"
#include "protonscanner.h"
#include "launcher.h"
#include "desktopfilewriter.h"
#include "iconextractor.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    KAboutData aboutData(
        QStringLiteral("vermouth"),
        i18n("Vermouth"),
        QStringLiteral(APP_VERSION_STRING),
        i18n("A no-frills Wine/Proton game launcher for Linux"),
        KAboutLicense::MIT,
        i18n("(c) 2024-2026")
    );
    aboutData.setBugAddress(QByteArrayLiteral("https://github.com/dekomote/vermouth/issues"));
    aboutData.setHomepage(QStringLiteral("https://github.com/dekomote/vermouth"));
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/vermouth.svg")));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    QCommandLineOption launchProtonOpt(QStringLiteral("launch-proton"), i18n("Launch exe with Proton"), QStringLiteral("exe"));
    QCommandLineOption launchWineOpt(QStringLiteral("launch-wine"), i18n("Launch exe with Wine"), QStringLiteral("exe"));
    QCommandLineOption protonOpt(QStringLiteral("proton"), i18n("Proton path"), QStringLiteral("path"));
    QCommandLineOption wineOpt(QStringLiteral("wine"), i18n("Wine binary path"), QStringLiteral("path"));
    QCommandLineOption prefixOpt(QStringLiteral("prefix"), i18n("Prefix path"), QStringLiteral("path"));

    parser.addOption(launchProtonOpt);
    parser.addOption(launchWineOpt);
    parser.addOption(protonOpt);
    parser.addOption(wineOpt);
    parser.addOption(prefixOpt);
    parser.addPositionalArgument(QStringLiteral("uri"), i18n("File or URI to open"), QStringLiteral("[uri...]"));
    parser.process(app);
    aboutData.processCommandLine(&parser);

    Launcher launcher;

    // Direct launch mode - no GUI
    if (parser.isSet(launchProtonOpt)) {
        QVariantMap entry;
        entry[QStringLiteral("runtimeType")] = QStringLiteral("proton");
        entry[QStringLiteral("protonPath")] = parser.value(protonOpt);
        entry[QStringLiteral("protonPrefix")] = parser.value(prefixOpt);
        entry[QStringLiteral("exePath")] = parser.value(launchProtonOpt);
        launcher.launchEntry(entry);
        return 0;
    }
    if (parser.isSet(launchWineOpt)) {
        QVariantMap entry;
        entry[QStringLiteral("runtimeType")] = QStringLiteral("wine");
        entry[QStringLiteral("wineBinary")] = parser.value(wineOpt);
        entry[QStringLiteral("winePrefix")] = parser.value(prefixOpt);
        entry[QStringLiteral("exePath")] = parser.value(launchWineOpt);
        launcher.launchEntry(entry);
        return 0;
    }

    AppModel appModel;
    ProtonScanner protonScanner;
    DesktopFileWriter desktopWriter;
    IconExtractor iconExtractor;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.rootContext()->setContextProperty(QStringLiteral("appModel"), &appModel);
    engine.rootContext()->setContextProperty(QStringLiteral("protonScanner"), &protonScanner);
    engine.rootContext()->setContextProperty(QStringLiteral("launcher"), &launcher);
    engine.rootContext()->setContextProperty(QStringLiteral("desktopWriter"), &desktopWriter);
    engine.rootContext()->setContextProperty(QStringLiteral("iconExtractor"), &iconExtractor);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
