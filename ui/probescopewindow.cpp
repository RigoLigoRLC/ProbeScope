
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
    m_dockWelcomeBackground = new ads::CDockWidget(tr("Welcome"), this);
    m_dockSymbolPanel = new ads::CDockWidget(tr("Symbols"), this);


    // Initialize panels
    m_welcomeBackground = new WelcomeBackground(m_dockWelcomeBackground);
    m_dockWelcomeBackground->setWidget(m_welcomeBackground);
    m_symbolPanel = new SymbolPanel(m_dockSymbolPanel);
    m_dockSymbolPanel->setWidget(m_symbolPanel);

    // Place dockable panels to dock manager
    m_dockMgr->setCentralWidget(m_dockWelcomeBackground);
    m_dockMgr->addDockWidget(ads::LeftDockWidgetArea, m_dockSymbolPanel);

    // Set up status bar
    // Use a widget to wrap around groups of objects to avoid vertical bars
    // Probe Selection
    auto probeWid = new QWidget();
    auto probeLay = new QHBoxLayout();
    m_lblConnection = new QLabel(tr("(No Probe Selected)"));
    m_lblConnection->setFixedWidth(200);
    m_btnSelectProbe = new QPushButton(tr("..."));
    m_btnSelectProbe->setFixedWidth(20);
    probeLay->setContentsMargins(0, 0, 0, 0);
    probeLay->setSpacing(0);
    probeLay->addWidget(m_lblConnection);
    probeLay->addWidget(m_btnSelectProbe);
    probeWid->setLayout(probeLay);
    probeWid->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    ui->statusbar->addWidget(probeWid);
    // Device Selection
    auto deviceWid = new QWidget();
    auto deviceLay = new QHBoxLayout();
    m_lblDevice = new QLabel(tr("(No Device Selected)"));
    m_lblDevice->setFixedWidth(250);
    m_btnSelectDevice = new QPushButton(tr("..."));
    m_btnSelectDevice->setFixedWidth(20);
    deviceLay->setContentsMargins(0, 0, 0, 0);
    deviceLay->setSpacing(0);
    deviceLay->addWidget(m_lblDevice);
    deviceLay->addWidget(m_btnSelectDevice);
    deviceWid->setLayout(deviceLay);
    deviceWid->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    ui->statusbar->addWidget(deviceWid);
    // Connection
    auto connWid = new QWidget();
    auto connLay = new QHBoxLayout();
    m_lblConnectionSpeed = new QLabel(tr("Disconnected"));
    m_lblConnectionSpeed->setFixedWidth(180);
    m_btnToggleConnection = new QPushButton();
    m_btnToggleConnection->setContentsMargins(0, 0, 0, 0);
    m_btnToggleConnection->setIcon(QIcon::fromTheme("connection-unconnected"));
    m_btnToggleConnection->setFixedWidth(40);
    m_btnToggleConnection->setCheckable(true);
    m_btnToggleConnection->setChecked(false);
    connLay->setContentsMargins(0, 0, 0, 0);
    connLay->setSpacing(0);
    connLay->addWidget(m_btnToggleConnection);
    connLay->addWidget(m_lblConnectionSpeed);
    connWid->setLayout(connLay);
    connWid->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    ui->statusbar->addWidget(connWid);

    // Initialize workspace
    m_workspace = new WorkspaceModel(this);

    m_symbolPanel->setSymbolBackend(m_workspace->getSymbolBackend());

    // Connect all the signals
    connect(m_btnSelectProbe, &QPushButton::clicked, this, &ProbeScopeWindow::sltSelectProbe);

    connect(m_symbolPanel->ui->btnOpenSymbolFile, &QPushButton::clicked, this, &ProbeScopeWindow::sltOpenSymbolFile);
    connect(m_symbolPanel->ui->btnReloadSymbolFile, &QPushButton::clicked, this,
            &ProbeScopeWindow::sltReloadSymbolFile);

    // Misc initialization
    reevaluateConnectionRelatedWidgetEnableStates();
}

ProbeScopeWindow::~ProbeScopeWindow() {
    delete ui;
}

void ProbeScopeWindow::reevaluateConnectionRelatedWidgetEnableStates() {
    auto probeLibHost = m_workspace->getProbeLibHost();
    bool connected = probeLibHost->sessionActive();
    bool probeBusy = false; // Will be true when a probe is used for data acquisition, etc.

    m_btnSelectDevice->setEnabled(probeLibHost->currentProbe() != nullptr);
    m_btnToggleConnection->setEnabled(probeLibHost->currentProbe() != nullptr && probeLibHost->currentDevice() != 0);

    ui->actionProbeBenchmark->setEnabled(connected);
}

Result<void, SymbolBackend::Error> ProbeScopeWindow::loadSymbolFile(QString symbolFileAbsPath) {
    // Clear symbol tree
    m_symbolPanel->ui->treeSymbolTree->clear();

    // TODO: Clear watch expressions

    auto result = m_workspace->loadSymbolFile(symbolFileAbsPath);
    if (result.isErr()) {
        m_symbolPanel->ui->btnReloadSymbolFile->setEnabled(false);
        QMessageBox::warning(this, tr("Symbol file failed to load"),
                             tr("Error message: %1.").arg(SymbolBackend::errorString(result.unwrapErr())));
        return Err(result.unwrapErr());
    }
    m_symbolPanel->ui->btnReloadSymbolFile->setEnabled(true);

    m_symbolPanel->buildRootFiles(m_workspace->getSymbolBackend());
    // FIXME:

    // Change display
    QFileInfo symFileInfo(symbolFileAbsPath);
    m_symbolPanel->ui->lblSymbolFileName->setText(symFileInfo.fileName());
    m_symbolPanel->ui->lblSymbolFileSizeAndDate->setText(
        QString("%1, %2").arg(ProbeScopeUtil::bytesToSize(symFileInfo.size(), 2),
                              symFileInfo.fileTime(QFile::FileModificationTime).toLocalTime().toString()));

    return Ok();
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

    loadSymbolFile(symbolFilePath);
}

void ProbeScopeWindow::sltReloadSymbolFile() {
    loadSymbolFile(m_workspace->getSymbolFilePath());
}

void ProbeScopeWindow::sltSelectProbe() {
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

        auto result = probeLibHost->selectProbe(probeLib, probe);
        if (result.isErr()) {
            QMessageBox::critical(this, tr("Failed to connect to probe"), result.unwrapErr());
            m_lblConnection->setText(tr("(No Probe Selected)"));
        } else {
            m_lblConnection->setText(tr("Probe: %1").arg(probe->name()));
        }
    }
    reevaluateConnectionRelatedWidgetEnableStates();
}
