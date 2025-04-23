
#include "watchentrypanel.h"
#include "models/watchentrymodel.h"

WatchEntryPanel::WatchEntryPanel(IUiStateBridge *uiBridge, QWidget *parent) : m_uiBridge(uiBridge), QWidget(parent) {
    ui = new Ui::WatchEntryPanel;
    ui->setupUi(this);

    m_plotAreaColumnDelegate = new PlotAreaColumnDelegate(uiBridge, this);
    ui->viewWatchEntry->setItemDelegateForColumn(WatchEntryModel::Columns::PlotAreas, m_plotAreaColumnDelegate);
    ui->viewWatchEntry->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->btnRemoveEntry, &QPushButton::clicked, this, &WatchEntryPanel::sltBtnRemoveEntryClicked);
}

WatchEntryPanel::~WatchEntryPanel() {
    delete ui;
}

void WatchEntryPanel::setModel(QAbstractItemModel *model) {
    ui->viewWatchEntry->setModel(model);
    connect(ui->viewWatchEntry->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &WatchEntryPanel::sltTableSelectionChanged);
}

void WatchEntryPanel::sltTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    // Fuck Qt documentation, they never said clearly the args are "change sets"
    for (auto &i : selected) {
        for (int row = i.top(); row <= i.bottom(); ++row) {
            m_selectedRows.insert(row);
        }
    }
    for (auto &i : deselected) {
        for (int row = i.top(); row <= i.bottom(); ++row) {
            m_selectedRows.remove(row);
        }
    }

    // If more than one row is selected, there must be valid rows
    // If only one row is selected, it must not be the last row (double click to add entry)
    ui->btnRemoveEntry->setEnabled(
        m_selectedRows.count() > 1 ||
        (m_selectedRows.count() == 1 && (*m_selectedRows.begin()) != (ui->viewWatchEntry->model()->rowCount() - 1)));
}

void WatchEntryPanel::sltBtnRemoveEntryClicked() {
    auto rowsList = m_selectedRows.values();
    std::sort(rowsList.begin(), rowsList.end(), std::ranges::greater());

    for (auto i : rowsList) {
        ui->viewWatchEntry->model()->removeRow(i);
    }
}
