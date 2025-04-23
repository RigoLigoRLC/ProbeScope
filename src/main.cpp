
#include "probescopewindow.h"
#include <QTranslator>
#include <qobject.h>
// #include <qtermwidget.h>
#include <singleapplication.h>


int main(int argc, char **argv) {
    SingleApplication app(argc, argv);

    // Add probelibs as a library path
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/probelibs");

    // Prepare for QSettings
    app.setOrganizationName("RigoLigoRLC");
    app.setOrganizationDomain("rigoligo.cc");
    app.setApplicationName("ProbeScope");

    QIcon::setThemeName("light"); // TODO: Make this configurable

    // qApp->setStyle("windowsvista"); // Fuck QWindows11Style

#ifdef Q_OS_WIN
    // Workaround for Qt5 font issue
    if (QLocale::system().language() == QLocale::Chinese) {
        auto currentFont = qApp->font();
        qApp->setFont(QFont("Microsoft YaHei UI", currentFont.pointSize(), currentFont.weight()));
    }
    // Add probelibs to PATH
    qputenv("PATH", qgetenv("PATH") + ";" + QCoreApplication::applicationDirPath().toUtf8() + "/probelibs");
#endif

    QTranslator translator;
    if (translator.load(QLocale(), "probescope", "_", ":/lang", ".qm")) {
        QCoreApplication::installTranslator(&translator);
    }

    ProbeScopeWindow window;
    window.show();

    return app.exec();
}
