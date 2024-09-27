
#include "probescopewindow.h"
#include "probelibhost.h"
#include "selectprobedialog.h"
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

    // Set up status bar
    // Use a widget to wrap around it to avoid vertical bars
    auto connectionWid = new QWidget();
    auto connectionLay = new QHBoxLayout();
    m_connectionLabel = new QLabel(tr("Not connected"));
    m_connectionLabel->setFixedWidth(250);
    m_modifyConnectionButton = new QPushButton(tr("..."));
    m_modifyConnectionButton->setFixedWidth(20);
    connectionLay->setContentsMargins(0, 0, 0, 0);
    connectionLay->addWidget(m_connectionLabel);
    connectionLay->addWidget(m_modifyConnectionButton);
    connectionWid->setLayout(connectionLay);
    connectionWid->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    ui->statusbar->addWidget(connectionWid);

    // Initialize workspace
    m_workspace = new WorkspaceModel(this);

    m_symbolPanel->setSymbolBackend(m_workspace->getSymbolBackend());

    // Connect all the signals
    connect(m_modifyConnectionButton, &QPushButton::clicked, this, &ProbeScopeWindow::sltModifyConnection);

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

void ProbeScopeWindow::sltModifyConnection() {
    // NOTE: MUST ensure that acquisition is not running
    SelectProbeDialog dialog(this);

    auto probeLibHost = m_workspace->getProbeLibHost();
    if (probeLibHost->probeLibs().isEmpty()) {
        QMessageBox::critical(this, tr("No probe libraries found"),
                              tr("No probe libraries found.\n"
                                 "ProbeScope is supposed to ship with a default probe library.\n"
                                 "Reinstalling ProbeScope may solve this issue."));
        return;
    }

    if (dialog.execWithState(probeLibHost->probeLibs(), probeLibHost->currentProbeLib(),
                             probeLibHost->currentProbe())) {
        auto probeLib = dialog.selectedProbeLib();
        auto probe = dialog.selectedProbe();

        auto result = probeLibHost->connect(probeLib, probe);
        if (result.isErr()) {
            QMessageBox::critical(this, tr("Failed to connect to probe"), result.unwrapErr());
            m_connectionLabel->setText(tr("Not connected"));
        } else {
            m_connectionLabel->setText(tr("Connected to %1").arg(probe->name()));
        }
    }
}
