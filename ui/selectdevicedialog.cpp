
#include "selectdevicedialog.h"

SelectDeviceDialog::SelectDeviceDialog(QWidget *parent) : QDialog(parent) {
    ui = new Ui::SelectDeviceDialog;
    ui->setupUi(this);
}

SelectDeviceDialog::~SelectDeviceDialog() {
    delete ui;
}
