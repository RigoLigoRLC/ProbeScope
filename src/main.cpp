
#include "probescopewindow.h"
#include <qobject.h>
#include <qtermwidget.h>
#include <singleapplication.h>

int main(int argc, char **argv) {
    SingleApplication app(argc, argv);

    // Prepare for QSettings
    app.setOrganizationName("RigoLigoRLC");
    app.setOrganizationDomain("rigoligo.cc");
    app.setApplicationName("ProbeScope");

    // Workaround for Qt5 font issue
#ifdef Q_OS_WIN
    if (QLocale::system().language() == QLocale::Chinese) {
        auto currentFont = qApp->font();
        qApp->setFont(QFont("Microsoft YaHei UI", currentFont.pointSize(), currentFont.weight()));
    }
#endif

    ProbeScopeWindow window;
    window.show();

    return app.exec();
}
