
#include "probescopewindow.h"
#include <psprobe/families.h>
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

    // size_t familyCount = 0, variantCount = 0;
    // InsertFamilies(*tree, familyCount, variantCount);
    // statusLabel->setText(QString("Families: %1, Chip variants: %2").arg(familyCount).arg(variantCount));
    // tree->sortItems(0, Qt::AscendingOrder);
    // statusBar->addWidget(statusLabel);

    ProbeScopeWindow window;
    window.show();

    return app.exec();
}
