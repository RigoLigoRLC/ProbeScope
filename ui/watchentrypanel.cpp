
#include "watchentrypanel.h"

WatchEntryPanel::WatchEntryPanel(QWidget *parent) : QWidget(parent) {
    ui = new Ui::WatchEntryPanel;
    ui->setupUi(this);
}

WatchEntryPanel::~WatchEntryPanel() {
    delete ui;
}
void WatchEntryPanel::setModel(QAbstractItemModel *model) {
    ui->viewWatchEntry->setModel(model);
}
