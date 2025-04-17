
#include "selectdevicedialog.h"

SelectDeviceDialog::SelectDeviceDialog(QWidget *parent) : QDialog(parent) {
    ui = new Ui::SelectDeviceDialog;
    ui->setupUi(this);

    m_model = std::make_unique<QStandardItemModel>();
    m_filterModel = std::make_unique<QSortFilterProxyModel>();

    m_filterModel->setSourceModel(m_model.get());
    m_filterModel->setRecursiveFilteringEnabled(true);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->radJtag->setChecked(true);
    ui->treeDevices->setModel(m_filterModel.get());

    connect(ui->treeDevices, &QTreeView::doubleClicked, this, &SelectDeviceDialog::itemDoubleClicked);
    connect(ui->edtSearch, &QLineEdit::textChanged, this, &SelectDeviceDialog::sltSearchTextChanged);
    connect(ui->btnSelect, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

SelectDeviceDialog::~SelectDeviceDialog() {
    delete ui;
}
bool SelectDeviceDialog::execWithState(const QVector<probelib::DeviceCategory> availableDevices,
                                       size_t currentDeviceId) {
    // Let user see the dialog
    show();
    QApplication::processEvents();

    // Use a simple stupid check to avoid repeated model actions
    if (m_availableDevices.data() != availableDevices.data()) {
        repopulateModel(availableDevices);
    }

    auto ret = exec();

    if (ret) {
        m_selectedDeviceIndex = ui->treeDevices->selectionModel()->currentIndex();
        m_selectedDeviceId = m_selectedDeviceIndex.data(DeviceIdRole).toULongLong();
    }

    return ret;
}

void SelectDeviceDialog::repopulateModel(const QVector<probelib::DeviceCategory> availableDevices) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_model->clear();

    for (auto &category : availableDevices) {
        auto categoryItem = new QStandardItem(category.name);
        categoryItem->setEditable(false);

        for (auto [id, name] : category.devices) {
            auto deviceItem = new QStandardItem(name);
            deviceItem->setData(id, DeviceIdRole);
            deviceItem->setEditable(false);
            categoryItem->appendRow(deviceItem);
        }

        m_model->appendRow(categoryItem);
    }

    QApplication::restoreOverrideCursor();
}

void SelectDeviceDialog::sltSearchTextChanged(QString str) {
    m_filterModel->setFilterFixedString(str);

    if (str.isEmpty()) {
        collapseAll();
    } else {
        expandAll();
    }
}

void SelectDeviceDialog::itemDoubleClicked(QModelIndex index) {
    if (index.parent().isValid()) {
        accept();
    }
}

void SelectDeviceDialog::collapseAll() {
    ui->treeDevices->collapseAll();
}

void SelectDeviceDialog::expandAll() {
    ui->treeDevices->expandAll();
}
