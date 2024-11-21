
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

private slots:
    void sltOpenSymbolFile();
    void sltReloadSymbolFile();

    void sltSelectProbe();

private:
    Ui::ProbeScopeWindow *ui;

    // Dockable components
    ads::CDockManager *m_dockMgr;
    ads::CDockWidget *m_dockWelcomeBackground;
    ads::CDockWidget *m_dockSymbolPanel;

    // Panel components
    WelcomeBackground *m_welcomeBackground;
    SymbolPanel *m_symbolPanel;

    // Status bar
    QLabel *m_lblConnection;
    QPushButton *m_btnSelectProbe;
    QLabel *m_lblDevice;
    QPushButton *m_btnSelectDevice;
    QPushButton *m_btnToggleConnection;
    QLabel *m_lblConnectionSpeed;

    // Backend
    WorkspaceModel *m_workspace;
};
