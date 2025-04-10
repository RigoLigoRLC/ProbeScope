
#pragma once

#include "probelib/iprobelib.h"
#include "ui_selectdevicedialog.h"
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <memory>

class SelectDeviceDialog : public QDialog {
public:
    SelectDeviceDialog(QWidget *parent);
    ~SelectDeviceDialog();

    enum Connection {
        Jtag,
        Swd,
    };

    bool execWithState(const QVector<probelib::DeviceCategory> availableDevices, size_t currentDeviceId);
    size_t selectedDevice() const { return m_selectedDeviceId; };
    QString selectedDeviceName() const { return m_selectedDeviceIndex.data().toString(); }
    Connection selectedConnection() const {
        return ui->radJtag->isChecked() ? Jtag : ui->radSwd->isChecked() ? Swd : Jtag;
    }

private:
    void repopulateModel(const QVector<probelib::DeviceCategory> availableDevices);
    void collapseAll();
    void expandAll();

private slots:
    void sltSearchTextChanged(QString str);
    void itemDoubleClicked(QModelIndex index);

private:
    enum Roles {
        DeviceIdRole = Qt::UserRole,
    };

    // Initial state
    QVector<probelib::DeviceCategory> m_availableDevices;
    size_t m_currentDeviceId = 0;

    // Modified state
    size_t m_selectedDeviceId = 0;

    // Remembered state
    QModelIndex m_selectedDeviceIndex;

    // Filter helper
    std::unique_ptr<QStandardItemModel> m_model;
    std::unique_ptr<QSortFilterProxyModel> m_filterModel;

private:
    Ui::SelectDeviceDialog *ui;
};
