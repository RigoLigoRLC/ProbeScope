
#pragma once

#include "models/watchentrymodel.h"
#include "qcustomplot.h"
#include "ui_plotareapanel.h"
#include <QMap>


class WorkspaceModel;

class PlotAreaPanel : public QWidget {
    Q_OBJECT
public:
    PlotAreaPanel(size_t areaId, WorkspaceModel *workspace, QWidget *parent = nullptr);
    ~PlotAreaPanel();

    void addPlot(size_t entryId);
    void removePlot(size_t entryId);
    QVector<size_t> assignedPlots() { return m_watchEntryToGraphMapping.keys().toVector(); }

    void replot() {
        // ui->plot->replot(QCustomPlot::rpQueuedReplot);
        ui->plot->xAxis->rescale();
        ui->plot->replot();
    }

private slots:
    void plotPropertyChanged(size_t entryId, WatchEntryModel::Columns prop, QVariant data);

private:
    size_t m_areaId;
    Ui::PlotAreaPanel *ui;

    WorkspaceModel *m_workspaceModel; ///< Backend reference

    // UI State
    QMap<size_t, QCPGraph *> m_watchEntryToGraphMapping; ///< (Watch entry ID -> QCPGraph mapping)
};
