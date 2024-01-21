
#include "symbolbackend.h"
#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <psprobe/families.h>
#include <qobject.h>
#include <qtermwidget.h>



#include "gdbmi.h"
#include "test_code.cpp.h"
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    QMainWindow window;
    QTreeWidget *tree = new QTreeWidget;
    QStatusBar *statusBar = new QStatusBar();
    QLabel *statusLabel = new QLabel();
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    size_t familyCount = 0, variantCount = 0;
    InsertFamilies(*tree, familyCount, variantCount);
    statusLabel->setText(QString("Families: %1, Chip variants: %2").arg(familyCount).arg(variantCount));
    tree->sortItems(0, Qt::AscendingOrder);
    statusBar->addWidget(statusLabel);

    auto gdbWidget = new QWidget;
    auto gdbWidgetLayout = new QVBoxLayout;
    auto gdbWidgetButtonLayout = new QHBoxLayout;
    auto gdbSetGdbExecutableButton = new QPushButton("Set GDB");
    auto gdbSetSymbolFileButton = new QPushButton("Set symbol file");
    auto gdbTerminal = new QTermWidget;
    SymbolBackend symbolBackend("");

    QObject::connect(gdbSetGdbExecutableButton, &QPushButton::clicked, [&]() {
        auto gdbPath = QFileDialog::getOpenFileName(nullptr, ("Select GDB executable"), QString(),
                                                    ("GDB executable (*.exe *.bat *.sh)"));
        if (gdbPath.isEmpty()) {
            return;
        }

        symbolBackend.setGdbExecutableLazy(gdbPath);
    });

    QObject::connect(gdbSetSymbolFileButton, &QPushButton::clicked, [&]() {
        auto symbolFileFullPath =
            QFileDialog::getOpenFileName(nullptr, ("Select symbol file"), QString(), ("Symbol file (*.elf *.out)"));
        if (symbolFileFullPath.isEmpty()) {
            return;
        }

        auto result = symbolBackend.switchSymbolFile(symbolFileFullPath);
        if (result != SymbolBackend::Error::NoError) {
            QMessageBox::critical(nullptr, ("Failed to switch symbol file"),
                                  ("Failed to switch symbol file. Please try again."));
        }
    });

    symbolBackend.getGdbTerminalDevice()->open(QIODevice::ReadOnly);
    auto terminalDevice = gdbTerminal->getPtyIo();
    QObject::connect(symbolBackend.getGdbTerminalDevice(), &QIODevice::readyRead, [&]() {
        auto data = symbolBackend.getGdbTerminalDevice()->readAll();
        terminalDevice->receiveDataFromBackend(data.constData(), data.size());
    });

    gdbWidgetButtonLayout->addWidget(gdbSetGdbExecutableButton);
    gdbWidgetButtonLayout->addWidget(gdbSetSymbolFileButton);

    gdbWidgetLayout->addLayout(gdbWidgetButtonLayout);
    gdbWidgetLayout->addWidget(gdbTerminal);

    gdbWidget->setLayout(gdbWidgetLayout);

    splitter->addWidget(gdbWidget);
    splitter->addWidget(tree);

    window.setCentralWidget(splitter);
    window.setStatusBar(statusBar);
    window.show();

    return app.exec();
}
