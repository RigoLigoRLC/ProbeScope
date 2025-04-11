
#include "probescopewindow.h"
#include "plotareapanel.h"
#include "probelibhost.h"
#include "selectprobedialog.h"
#include "symbolbackend.h"
#include "utils.h"
#include "workspacemodel.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

ProbeScopeWindow::ProbeScopeWindow(QWidget *parent) : QMainWindow(parent), m_refreshTimerShouldStop(false) {
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
    m_dockWatchEntryPanel = new ads::CDockWidget(tr("Watch Entries"), this);

    // Initialize panels
    m_welcomeBackground = new WelcomeBackground(m_dockWelcomeBackground);
    m_dockWelcomeBackground->setWidget(m_welcomeBackground);
    m_symbolPanel = new SymbolPanel(m_dockSymbolPanel);
    m_dockSymbolPanel->setWidget(m_symbolPanel, ads::CDockWidget::ForceNoScrollArea);
    m_watchEntryPanel = new WatchEntryPanel(m_dockWatchEntryPanel);
    m_dockWatchEntryPanel->setWidget(m_watchEntryPanel, ads::CDockWidget::ForceNoScrollArea);

    // Place dockable panels to dock manager
    m_dockMgr->setCentralWidget(m_dockWelcomeBackground);
    m_dockMgr->addDockWidget(ads::LeftDockWidgetArea, m_dockSymbolPanel);
    m_dockMgr->addDockWidget(ads::BottomDockWidgetArea, m_dockWatchEntryPanel);

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
    m_watchEntryPanel->setModel(m_workspace->getWatchEntryModel());

    // Connect all the signals
    connect(m_btnSelectProbe, &QPushButton::clicked, this, &ProbeScopeWindow::sltSelectProbe);
    connect(m_btnSelectDevice, &QPushButton::clicked, this, &ProbeScopeWindow::sltSelectDevice);
    connect(m_btnToggleConnection, &QPushButton::clicked, this, &ProbeScopeWindow::sltToggleConnection);

    connect(m_symbolPanel->ui->btnOpenSymbolFile, &QPushButton::clicked, this, &ProbeScopeWindow::sltOpenSymbolFile);
    connect(m_symbolPanel->ui->btnReloadSymbolFile, &QPushButton::clicked, this,
            &ProbeScopeWindow::sltReloadSymbolFile);
    connect(m_symbolPanel, &SymbolPanel::addWatchExpression,
            [&](QString expr) { m_workspace->addWatchEntry(expr, {}); });

    connect(m_workspace, &WorkspaceModel::requestAddPlotArea, this, &ProbeScopeWindow::sltCreatePlotArea);
    connect(m_workspace, &WorkspaceModel::requestRemovePlotArea, this, &ProbeScopeWindow::sltRemovePlotArea);
    connect(m_workspace, &WorkspaceModel::requestAssignGraphOnPlotArea, this,
            &ProbeScopeWindow::sltAssignGraphOnPlotArea);
    connect(m_workspace, &WorkspaceModel::requestUnassignGraphOnPlotArea, this,
            &ProbeScopeWindow::sltUnassignGraphOnPlotArea);
    connect(m_workspace, &WorkspaceModel::feedbackAcquisitionStopped, this,
            &ProbeScopeWindow::sltAcquisitionThreadStopped);

    // Actions
    connect(ui->actionStartAcquisition, &QAction::triggered, this, &ProbeScopeWindow::sltStartAcquisition);
    connect(ui->actionStopAcquisition, &QAction::triggered, this, &ProbeScopeWindow::sltStopAcquisition);

    // UI internal signals
    connect(&m_refreshTimer, &QTimer::timeout, this, &ProbeScopeWindow::sltRefreshTimerExpired);

    // Init long-living dialogs
    m_selectDeviceDialog = new SelectDeviceDialog(this);
    m_selectProbeDialog = new SelectProbeDialog(this);

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

    // TODO: Lock all buttons when a probe session is locked
    m_btnSelectDevice->setEnabled(probeLibHost->currentProbe() != nullptr);
    m_btnToggleConnection->setEnabled((probeLibHost->currentProbe() != nullptr) &&
                                      (probeLibHost->currentDevice() != 0));

    ui->actionProbeBenchmark->setEnabled(connected);
    m_btnToggleConnection->setIcon(QIcon::fromTheme(connected ? "connection-connected" : "connection-unconnected"));
    m_lblConnectionSpeed->setText(connected ? tr("Connected") : tr("Unconnected"));
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

void ProbeScopeWindow::sltStartAcquisition() {
    m_workspace->notifyAcquisitionStarted();
    m_refreshTimer.start(); // FIXME: This timer should be started and stopped based on workspace's state signals
}

void ProbeScopeWindow::sltStopAcquisition() {
    m_workspace->notifyAcquisitionStopped();
}

void ProbeScopeWindow::sltSelectProbe() {
    // NOTE: MUST ensure that acquisition is not running

    auto probeLibHost = m_workspace->getProbeLibHost();
    if (probeLibHost->probeLibs().isEmpty()) {
        QMessageBox::critical(this, tr("No probe libraries found"),
                              tr("No probe libraries found.\n"
                                 "ProbeScope is supposed to ship with a default probe library.\n"
                                 "Reinstalling ProbeScope may solve this issue."));
        return;
    }

    if (m_selectProbeDialog->execWithState(probeLibHost->probeLibs(), probeLibHost->currentProbeLib(),
                                           probeLibHost->currentProbe())) {
        auto probeLib = m_selectProbeDialog->selectedProbeLib();
        auto probe = m_selectProbeDialog->selectedProbe();

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

void ProbeScopeWindow::sltSelectDevice() {
    auto probeLibHost = m_workspace->getProbeLibHost();
    Q_ASSERT(probeLibHost->currentProbeLib());

    auto devices = probeLibHost->currentProbeLib()->supportedDevices();
    auto currentDevice = probeLibHost->currentDevice();

    if (m_selectDeviceDialog->execWithState(devices, currentDevice)) {
        auto result = probeLibHost->selectDevice(m_selectDeviceDialog->selectedDevice());
        if (result.isErr()) {
            QMessageBox::critical(this, tr("Failed to select device"), result.unwrapErr());
            m_lblDevice->setText(tr("No Device Selected"));
        } else {
            m_lblDevice->setText(tr("Device: %1").arg(m_selectDeviceDialog->selectedDeviceName()));
        }
    }
    reevaluateConnectionRelatedWidgetEnableStates();
};

void ProbeScopeWindow::sltToggleConnection() {
    //
    auto probeLibHost = m_workspace->getProbeLibHost();

    Q_ASSERT(probeLibHost);

    if (!probeLibHost->sessionActive()) {
        if (auto result = probeLibHost->connect(); result.isErr()) {
            QMessageBox::critical(this, tr("Connect fail"), result.unwrapErr());
            m_btnToggleConnection->setChecked(false);
        }
    } else {
        probeLibHost->disconnect();
    }
    reevaluateConnectionRelatedWidgetEnableStates();
}

void ProbeScopeWindow::sltCreatePlotArea(size_t id) {
    Q_ASSERT(!m_dockPlotAreas.contains(id) && !m_plotAreas.contains(id));

    auto area = new PlotAreaPanel(id, m_workspace);
    auto dock = new ads::CDockWidget(tr("Plot area %1").arg(id), this);
    dock->setWidget(area, ads::CDockWidget::ForceNoScrollArea);

    m_dockMgr->addDockWidgetTab(ads::CenterDockWidgetArea, dock);
    m_dockPlotAreas[id] = dock;
    m_plotAreas[id] = area;
}

void ProbeScopeWindow::sltRemovePlotArea(size_t id) {
    Q_ASSERT(m_dockPlotAreas.contains(id) && m_plotAreas.contains(id));

    auto area = m_plotAreas.take(id);
    auto dock = m_dockPlotAreas.take(id);

    auto plots = area->assignedPlots();
    for (auto entryId : plots) {
        auto areasResult = m_workspace->getWatchEntryGraphProperty(entryId, WatchEntryModel::PlotAreas);
        Q_ASSERT(areasResult.isOk());
        auto newAreas = areasResult.unwrap().value<QSet<size_t>>();
        newAreas.remove(id);
        m_workspace->setWatchEntryGraphProperty(entryId, WatchEntryModel::PlotAreas, QVariant::fromValue(newAreas));
    }
}

void ProbeScopeWindow::sltAssignGraphOnPlotArea(size_t entryId, size_t areaId) {
    Q_ASSERT(m_dockPlotAreas.contains(areaId) && m_plotAreas.contains(areaId));

    m_plotAreas[areaId]->addPlot(entryId);
}

void ProbeScopeWindow::sltUnassignGraphOnPlotArea(size_t entryId, size_t areaId) {
    Q_ASSERT(m_dockPlotAreas.contains(areaId) && m_plotAreas.contains(areaId));

    m_plotAreas[areaId]->removePlot(entryId);
}

void ProbeScopeWindow::sltAcquisitionThreadStopped() {
    m_refreshTimerShouldStop = true;
}

void ProbeScopeWindow::sltRefreshTimerExpired() {
    // Notify the workspace to pull acquisition data
    auto pulledData = m_workspace->pullBufferedAcquisitionData();

    // Refresh UI
    foreach (auto &i, m_plotAreas) {
        i->replot();
    }

    // Check stop flag
    if (!pulledData && m_refreshTimerShouldStop) {
        m_refreshTimer.stop();
        m_refreshTimerShouldStop = false;
    }
}
