
#pragma once

#include "ui_selectdevicedialog.h"

class SelectDeviceDialog : public QDialog {
public:
    SelectDeviceDialog(QWidget *parent);
    ~SelectDeviceDialog();

private:
    Ui::SelectDeviceDialog *ui;
};
