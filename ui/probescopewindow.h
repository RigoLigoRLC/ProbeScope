
#pragma once

#include "symbolpanel.h"
#include "ui_probescopewindow.h"
#include "workspacemodel.h"
#include <DockManager.h>
#include <DockWidget.h>

class ProbeScopeWindow : public QMainWindow {
    Q_OBJECT
public:
    ProbeScopeWindow(QWidget *parent = nullptr);
    ~ProbeScopeWindow();

private slots:
    void sltOpenSymbolFile();
    void sltReloadSymbolFile();

private:
    Ui::ProbeScopeWindow *ui;

    // Dockable components
    ads::CDockManager *m_dockMgr;
    ads::CDockWidget *m_dockSymbolPanel;

    // Panel components
    SymbolPanel *m_symbolPanel;

    // Backend
    WorkspaceModel *m_workspace;
};
