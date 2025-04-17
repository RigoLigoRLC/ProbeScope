
#include "plotareapanel.h"
#include "workspacemodel.h"

PlotAreaPanel::PlotAreaPanel(size_t areaId, WorkspaceModel *workspace, QWidget *parent)
    : m_areaId(areaId), m_workspaceModel(workspace), QWidget(parent) {
    ui = new Ui::PlotAreaPanel;
    ui->setupUi(this);

    ui->plot->yAxis->setRange(-10, 10);
}

PlotAreaPanel::~PlotAreaPanel() {
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

void PlotAreaPanel::plotPropertyChanged(size_t entryId, WatchEntryModel::Columns prop, QVariant data) {
    // TODO: respond to changes on UI
}
