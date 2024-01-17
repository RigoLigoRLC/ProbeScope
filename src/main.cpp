
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QtWidgets/qboxlayout.h>
#include <psprobe/families.h>

#include "gdbmi.h"
#include "test_code.cpp.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    QMainWindow window;
    QTreeWidget tree(&window);
    QStatusBar statusBar;
    QLabel statusLabel;
    QSplitter splitter(Qt::Horizontal);

    size_t familyCount = 0, variantCount = 0;
    InsertFamilies(tree, familyCount, variantCount);
    statusLabel.setText(QString("Families: %1, Chip variants: %2").arg(familyCount).arg(variantCount));
    tree.sortItems(0, Qt::AscendingOrder);
    statusBar.addWidget(&statusLabel);

    QPlainTextEdit textEdit;
    QPushButton parseButton;
    QVBoxLayout parseLayout;
    QWidget parseWidget;

    parseButton.setText("Parse");
    QObject::connect(&parseButton, &QPushButton::clicked, [&]() {
        auto result = gdbmi::parse_response(textEdit.toPlainText());
        result = result;
    });
    parseWidget.setLayout(&parseLayout);
    parseLayout.addWidget(&parseButton);
    parseLayout.addWidget(&textEdit);

    splitter.addWidget(&parseWidget);
    splitter.addWidget(&tree);

    window.setCentralWidget(&splitter);
    window.setStatusBar(&statusBar);
    window.show();

    return app.exec();
}
