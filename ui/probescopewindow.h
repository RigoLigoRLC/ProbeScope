
#pragma once

#include "acquisitionhub.h"
#include "plotareapanel.h"
#include "selectdevicedialog.h"
#include "selectprobedialog.h"
#include "symbolpanel.h"
#include "ui_probescopewindow.h"
#include "uistatebridge.h"
#include "watchentrypanel.h"
#include "welcomebackground.h"
#include "workspacemodel.h"
#include <DockManager.h>
#include <DockWidget.h>
#include <QTimer>

using namespace Qt::StringLiterals;

class ProbeScopeWindow : public QMainWindow, public IUiStateBridge {
    Q_OBJECT
public:
    ProbeScopeWindow(QWidget *parent = nullptr);
    ~ProbeScopeWindow();

    // IUiStateBridge interfaces
    virtual QMap<size_t, QString> collectPlotAreaNames() override;
    virtual QString getPlotAreaName(size_t areaId) override;

public:
    enum DockWidgetType {
        DWT_Welcome,
        DWT_Symbols,
        DWT_WatchEntries,
        DWT_PlotArea,
    };
    Q_ENUM(DockWidgetType);

private:
    void setConnectionSpeed(int khz); // Negative for unconnected

    // State management
    void addDockWidget(ads::DockWidgetArea area, ads::CDockWidget *dWidget);
    void addDockWidgetTab(ads::DockWidgetArea area, ads::CDockWidget *dWidget);
    void reevaluateConnectionRelatedWidgetEnableStates();

    // Inner utils
    Result<void, SymbolBackend::Error> loadSymbolFile(QString symbolFileAbsPath);

private:
    Ui::ProbeScopeWindow *ui;

    // Dockable components
    ads::CDockManager *m_dockMgr;
    ads::CDockWidget *m_dockWelcomeBackground;
    ads::CDockWidget *m_dockSymbolPanel;
    ads::CDockWidget *m_dockWatchEntryPanel;
    QMap<size_t, ads::CDockWidget *> m_dockPlotAreas;

    // Panel components
    WelcomeBackground *m_welcomeBackground;
    SymbolPanel *m_symbolPanel;
    WatchEntryPanel *m_watchEntryPanel;
    QMap<size_t, PlotAreaPanel *> m_plotAreas;

    // Status bar
    QLabel *m_lblConnection;
    QPushButton *m_btnSelectProbe;
    QLabel *m_lblDevice;
    QPushButton *m_btnSelectDevice;
    QPushButton *m_btnToggleConnection;
    QLabel *m_lblConnectionSpeed;

    // Long-living dialogs
    SelectProbeDialog *m_selectProbeDialog;
    SelectDeviceDialog *m_selectDeviceDialog;

    // UI Bookkeeping
    QTimer m_refreshTimer;         ///< Timer for refreshing plot view
    bool m_refreshTimerShouldStop; ///< Timer stop flag. If the flag is set and not data is pulled, timer is stopped.

    // Backend, where all the important stuff happens
    WorkspaceModel *m_workspace;

private slots:
    // Window
    void sltOpenSymbolFile();
    void sltReloadSymbolFile();

    // Actions
    void sltStartAcquisition();
    void sltStopAcquisition();
    void sltNewPlotArea();

    // Status bar
    void sltSelectProbe();
    void sltSelectDevice();
    void sltToggleConnection();

    // From backend
    void sltCreatePlotArea(size_t id);
    void sltRemovePlotArea(size_t id);
    void sltAssignGraphOnPlotArea(size_t entryId, size_t areaId);
    void sltUnassignGraphOnPlotArea(size_t entryId, size_t areaId);

    void sltAcquisitionThreadStopped();

    // UI Internal
    void sltRefreshTimerExpired();
    void sltDockWidgetClosed();
    void sltDockWidgetRemoved(ads::CDockWidget *dWidget);
    void sltFocusedDockWidgetChanged(ads::CDockWidget *old, ads::CDockWidget *now);

signals:
    void plotAreaClosed(size_t id);
};
