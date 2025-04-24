
#include "plotareapanel.h"
#include "plotter/axistickertimems.h"
#include "ui_plotareapanel.h"
#include "workspacemodel.h"


PlotAreaPanel::PlotAreaPanel(size_t areaId, WorkspaceModel *workspace, QWidget *parent)
    : m_areaId(areaId), ui(new Ui::PlotAreaPanel), m_workspaceModel(workspace), m_yTickerLog(new QCPAxisTickerLog),
      QWidget(parent) {
    ui->setupUi(this);
    m_adjustPopup = new PlotAreaAdjustPopup();
    m_adjustPopup->hide();

    // Preserve the linear Y axis ticker
    m_yTickerNormal = ui->plot->yAxis->ticker();

    QSharedPointer<AxisTickerTimeMs> xTicker(new AxisTickerTimeMs);
    xTicker->setTimeFormat("%m:%s.%z");
    ui->plot->xAxis->setTicker(xTicker);

    connect(m_adjustPopup, &PlotAreaAdjustPopup::horizontalZoomChanged, this, &PlotAreaPanel::sltHorizontalZoomChanged);
    connect(m_adjustPopup, &PlotAreaAdjustPopup::verticalZoomChanged, this, &PlotAreaPanel::sltVerticalZoomChanged);
    connect(m_adjustPopup, &PlotAreaAdjustPopup::lostFocus, this, &PlotAreaPanel::sltAdjustWindowLostFocus);

    // ＦＵＣＫ　ＯＶＥＲＬＯＡＤＥＤ　ＳＩＧＮＡＬＳ　ＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡＡ
    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(sltPlotHorizRageChanged(QCPRange)));

    sltHorizontalZoomChanged();
    sltVerticalZoomChanged();

    m_currentObservedXMax = 0;
}

PlotAreaPanel::~PlotAreaPanel() {
    delete m_adjustPopup;
    delete ui;
}

void PlotAreaPanel::addPlot(size_t entryId) {
    if (m_watchEntryToGraphMapping.contains(entryId)) {
        qWarning() << "Watch entry" << entryId << "already owned by plot area" << m_areaId;
        return;
    }

    auto graph = ui->plot->addGraph();

    // Fetch data and properties from backend
    auto dataContainerResult = m_workspaceModel->getWatchEntryDataContainer(entryId);
    if (dataContainerResult.isErr()) {
        qCritical() << "Cannot get data container for entry" << entryId << ":" << dataContainerResult.unwrapErr();
        return;
    }

    graph->setData(dataContainerResult.unwrap());

    // TODO: set properties
    // Set plot style things
    auto pen = graph->pen();
    pen.setStyle(m_workspaceModel->getWatchEntryGraphProperty(entryId, WatchEntryModel::LineStyle)
                     .unwrap()
                     .value<Qt::PenStyle>());
    pen.setColor(
        m_workspaceModel->getWatchEntryGraphProperty(entryId, WatchEntryModel::Color).unwrap().value<QColor>());
    pen.setWidth(m_workspaceModel->getWatchEntryGraphProperty(entryId, WatchEntryModel::Thickness).unwrap().toInt());
    graph->setPen(pen);

    m_watchEntryToGraphMapping[entryId] = graph;
}

void PlotAreaPanel::removePlot(size_t entryId) {
    if (!m_watchEntryToGraphMapping.contains(entryId)) {
        // This signal is broadcast to every plot area panel, don't panic if this entry not found
        return;
    }

    auto graph = m_watchEntryToGraphMapping.take(entryId);
    ui->plot->removeGraph(graph);

    return;
}
void PlotAreaPanel::replot() {
    if (m_horizAutoFit && m_vertAutoFit) {
        ui->plot->rescaleAxes();
    } else {
        if (m_horizAutoFit) {
            ui->plot->xAxis->rescale();
        } else {
            // Horizontal scrollbar maximum should be determined on our own :C
            auto range = ui->plot->xAxis->range();
            double max = 0.0;
            for (auto &graph : ui->plot->xAxis->plottables()) {
                bool foundRange;
                max = std::max(max, graph->getKeyRange(foundRange).upper);
            }
            m_currentObservedXMax = max;

            if (range.size() > max) {
                ui->scrollHorizontal->setMaximum(ui->scrollHorizontal->minimum());
            } else {
                ui->scrollHorizontal->setMaximum(qRound((max - range.size() / 2) / PlotToScrollBarCoeff));
            }

            if (m_horizAutoScroll) {
                // qDebug() << "Observed Max X:" << m_currentObservedXMax << "RangeSize" << range.size();

                // If current data can't fill the selected X range, let the graph snap to the left bound of the view
                if (range.size() > max) {
                    ui->plot->xAxis->setRange(0, range.size());
                } else {
                    ui->plot->xAxis->setRange(max - range.size(), max);
                }
            }
        }

        if (m_vertAutoFit) {
            ui->plot->yAxis->rescale();
        }
    }


    // ui->plot->replot(QCustomPlot::rpQueuedReplot);
    ui->plot->replot();
}

void PlotAreaPanel::sltPlotHorizRageChanged(const QCPRange range) {
    ui->scrollHorizontal->setPageStep(qRound(range.size() / PlotToScrollBarCoeff));
    ui->scrollHorizontal->setValue(qRound(range.center() / PlotToScrollBarCoeff));

    // Because the scroll bar value is center, half of the screen should be cut off from scrollbar's reach
    ui->scrollHorizontal->setMinimum(qRound(range.size() / 2 / PlotToScrollBarCoeff));
}

void PlotAreaPanel::plotPropertyChanged(size_t entryId, WatchEntryModel::Columns prop, QVariant data) {
    auto findResult = m_watchEntryToGraphMapping.find(entryId);
    if (findResult == m_watchEntryToGraphMapping.end()) {
        return;
    }

    auto &entry = *findResult;
    switch (prop) {
        case WatchEntryModel::Color: {
            auto pen = entry->pen();
            pen.setColor(data.value<QColor>());
            entry->setPen(pen);
            break;
        }
        case WatchEntryModel::DisplayName: {
            entry->setName(data.toString());
            break;
        }
        case WatchEntryModel::Thickness: {
            auto pen = entry->pen();
            pen.setWidth(data.toInt());
            entry->setPen(pen);
            break;
        }
        case WatchEntryModel::LineStyle: {
            auto pen = entry->pen();
            pen.setStyle(data.value<Qt::PenStyle>());
            entry->setPen(pen);
            break;
        }
        // Below are just not what we need to care about
        case WatchEntryModel::Expression:
        case WatchEntryModel::FrequencyLimit:
        case WatchEntryModel::PlotAreas:
        case WatchEntryModel::MaxColumns:
        case WatchEntryModel::FrequencyFeedback:
        case WatchEntryModel::ExpressionOkay: break;
    }
}

void PlotAreaPanel::on_btnAdjust_clicked() {
    m_adjustPopup->show();

    auto geom = m_adjustPopup->geometry();
    geom.moveBottomRight(mapToGlobal(geometry().bottomRight()));
    m_adjustPopup->setGeometry(geom);
}

void PlotAreaPanel::on_chkAutoscroll_clicked() {
    m_horizAutoScroll = ui->chkAutoscroll->isChecked();
    sltHorizontalZoomChanged();
}

void PlotAreaPanel::on_scrollHorizontal_valueChanged(int value) {
    // Taken from QCustomPlot examples
    // if user is dragging plot, we don't want to replot twice
    if (qAbs(ui->plot->xAxis->range().center() - value * PlotToScrollBarCoeff) > 0.01) {
        ui->plot->xAxis->setRange(value * PlotToScrollBarCoeff, ui->plot->xAxis->range().size(), Qt::AlignCenter);
        ui->plot->replot();
    }
}

void PlotAreaPanel::sltHorizontalZoomChanged() {
    bool autoFit;
    double rangeSecs;
    m_adjustPopup->getHorizontalRange(autoFit, rangeSecs);

    m_horizAutoFit = autoFit;

    ui->scrollHorizontal->setDisabled(m_horizAutoFit);

    // try to keep the current center
    // If the destination range exceeds the whole [0, observedXMax], place left bound at zero
    auto range = ui->plot->xAxis->range();
    if (rangeSecs * SecToPlotCoeff > m_currentObservedXMax || range.center() < rangeSecs * SecToPlotCoeff / 2) {
        ui->plot->xAxis->setRange(0, rangeSecs * SecToPlotCoeff);
    } else {
        QCPRange newRange{range.center() - rangeSecs / 2 * SecToPlotCoeff,
                          range.center() + rangeSecs / 2 * SecToPlotCoeff};
        ui->plot->xAxis->setRange(newRange);
    }

    replot();
}

void PlotAreaPanel::sltVerticalZoomChanged() {
    bool autoFit, logScale;
    double rangeL, rangeH;
    m_adjustPopup->getVerticalRange(logScale, autoFit, rangeL, rangeH);

    m_vertAutoFit = autoFit;

    auto scaleType = logScale ? QCPAxis::stLogarithmic : QCPAxis::stLinear;
    if (ui->plot->yAxis->scaleType() != scaleType) {
        ui->plot->yAxis->setScaleType(scaleType);
        ui->plot->yAxis->setTicker(logScale ? m_yTickerLog : m_yTickerNormal);
    }

    if (!autoFit) {
        ui->plot->yAxis->setRange(rangeL, rangeH);
    }

    replot();
}

void PlotAreaPanel::sltAdjustWindowLostFocus() {
    m_adjustPopup->hide();
}
