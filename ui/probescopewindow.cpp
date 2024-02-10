
#include "probescopewindow.h"
#include "symbolbackend.h"
#include "utils.h"
#include "workspacemodel.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>



ProbeScopeWindow::ProbeScopeWindow(QWidget *parent) : QMainWindow(parent) {
    ui = new Ui::ProbeScopeWindow;
    ui->setupUi(this);

    // Initialize dock manager
    ads::CDockManager::setConfigFlag(ads::CDockManager::DefaultOpaqueConfig, true);
    ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);

    m_dockMgr = new ads::CDockManager(this);
    m_dockSymbolPanel = new ads::CDockWidget(tr("Symbols"), this);

    // Initialize panels
    m_symbolPanel = new SymbolPanel(m_dockSymbolPanel);
    m_dockSymbolPanel->setWidget(m_symbolPanel);

    // Place dockable panels to dock manager
    m_dockMgr->addDockWidget(ads::LeftDockWidgetArea, m_dockSymbolPanel);

    // Initialize workspace
    m_workspace = new WorkspaceModel(this);

    m_symbolPanel->setSymbolBackend(m_workspace->getSymbolBackend());

    // Connect all the signals
    connect(m_symbolPanel->ui->btnOpenSymbolFile, &QPushButton::clicked, this, &ProbeScopeWindow::sltOpenSymbolFile);
    connect(m_symbolPanel->ui->btnReloadSymbolFile, &QPushButton::clicked, this,
            &ProbeScopeWindow::sltReloadSymbolFile);
}

ProbeScopeWindow::~ProbeScopeWindow() {
    delete ui;
}

void ProbeScopeWindow::sltOpenSymbolFile() {
    QSettings settings;
    QString symbolFilePath = QFileDialog::getOpenFileName(this, tr("Open symbol file..."),
                                                          settings.value("SavedPaths/OpenSymbolDir").toString(),
                                                          tr("ELF archives (*.axf *.elf)"));

    if (symbolFilePath.isEmpty()) {
        return;
    }

    QFileInfo symFileInfo(symbolFilePath);
    if (symFileInfo.dir().exists()) {
        settings.setValue("SavedPaths/OpenSymbolDir", symFileInfo.dir().absolutePath());
    }

    // Clear symbol tree
    m_symbolPanel->ui->treeSymbolTree->clear();

    // TODO: Clear watch expressions

    auto result = m_workspace->loadSymbolFile(symbolFilePath);
    if (result.isErr()) {
        QMessageBox::warning(this, tr("Symbol file failed to load"),
                             tr("Error message: %1.").arg(SymbolBackend::errorString(result.unwrapErr())));
        return;
    }

    m_symbolPanel->buildRootFiles(m_workspace->getSymbolBackend());
    // FIXME:

    // Change display
    m_symbolPanel->ui->lblSymbolFileName->setText(symFileInfo.fileName());
    m_symbolPanel->ui->lblSymbolFileSizeAndDate->setText(
        QString("%1, %2").arg(ProbeScopeUtil::bytesToSize(symFileInfo.size(), 2),
                              symFileInfo.fileTime(QFile::FileModificationTime).toLocalTime().toString()));
}

void ProbeScopeWindow::sltReloadSymbolFile() {}
