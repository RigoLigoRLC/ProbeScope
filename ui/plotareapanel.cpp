
#include "plotareapanel.h"
#include "workspacemodel.h"

PlotAreaPanel::PlotAreaPanel(size_t areaId, WorkspaceModel *workspace, QWidget *parent)
    : m_areaId(areaId), m_workspaceModel(workspace), QWidget(parent) {
    ui = new Ui::PlotAreaPanel;
    ui->setupUi(this);
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
