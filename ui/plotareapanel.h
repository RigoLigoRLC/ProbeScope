
#pragma once

#include "models/watchentrymodel.h"
#include "plotareaadjustpopup.h"
#include "qcustomplot.h"
#include <QMap>

class WorkspaceModel;
namespace Ui {
class PlotAreaPanel;
}

class PlotAreaPanel : public QWidget {
    Q_OBJECT
public:
    PlotAreaPanel(size_t areaId, WorkspaceModel *workspace, QWidget *parent = nullptr);
    ~PlotAreaPanel();

    void addPlot(size_t entryId);
    void removePlot(size_t entryId);
    QVector<size_t> assignedPlots() { return m_watchEntryToGraphMapping.keys().toVector(); }

    void replot();

    static constexpr double PlotToScrollBarCoeff = 10.0, SecToPlotCoeff = 1000.0;
private slots:
    void sltPlotHorizRageChanged(const QCPRange range);

    void plotPropertyChanged(size_t entryId, WatchEntryModel::Columns prop, QVariant data);
    void on_btnAdjust_clicked();
    void on_chkAutoscroll_clicked();
    void on_scrollHorizontal_valueChanged(int value);

    void sltHorizontalZoomChanged();
    void sltVerticalZoomChanged();
    void sltAdjustWindowLostFocus();

private:
    size_t m_areaId;
    Ui::PlotAreaPanel *ui;

    PlotAreaAdjustPopup *m_adjustPopup;
    WorkspaceModel *m_workspaceModel; ///< Backend reference

    QSharedPointer<QCPAxisTicker> m_yTickerNormal, m_yTickerLog;
    double m_currentObservedXMax;

    // UI State
    QMap<size_t, QCPGraph *> m_watchEntryToGraphMapping; ///< (Watch entry ID -> QCPGraph mapping)

    bool m_horizAutoFit;
    bool m_horizAutoScroll;
    bool m_vertAutoFit;
};
