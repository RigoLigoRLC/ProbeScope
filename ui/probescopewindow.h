
#pragma once

#include "symbolpanel.h"
#include "ui_probescopewindow.h"
#include "welcomebackground.h"
#include "workspacemodel.h"
#include <DockManager.h>
#include <DockWidget.h>

class ProbeScopeWindow : public QMainWindow {
    Q_OBJECT
public:
    ProbeScopeWindow(QWidget *parent = nullptr);
    ~ProbeScopeWindow();

private:
    void setConnectionSpeed(int khz); // Negative for unconnected

    // State management
    void reevaluateConnectionRelatedWidgetEnableStates();

    // Inner utils
    Result<void, SymbolBackend::Error> loadSymbolFile(QString symbolFileAbsPath);

private:
    Ui::ProbeScopeWindow *ui;

    // Dockable components
    ads::CDockManager *m_dockMgr;
    ads::CDockWidget *m_dockWelcomeBackground;
    ads::CDockWidget *m_dockSymbolPanel;
    QMap<size_t, ads::CDockWidget *> m_dockPlotAreas;

    // Panel components
    WelcomeBackground *m_welcomeBackground;
    SymbolPanel *m_symbolPanel;
    QMap<size_t, QWidget *> m_plotAreas; // TODO: Correct type for plot area

    // Status bar
    QLabel *m_lblConnection;
    QPushButton *m_btnSelectProbe;
    QLabel *m_lblDevice;
    QPushButton *m_btnSelectDevice;
    QPushButton *m_btnToggleConnection;
    QLabel *m_lblConnectionSpeed;

    // Backend
    WorkspaceModel *m_workspace;

private slots:
    void sltOpenSymbolFile();
    void sltReloadSymbolFile();

    void sltSelectProbe();

    void sltCreatePlotArea(size_t id);

signals:
    void plotAreaClosed(size_t id);
};
