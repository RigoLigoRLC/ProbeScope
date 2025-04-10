
#pragma once

#include "ui_watchentrypanel.h"
#include <QWidget>

class QAbstractItemModel;
class WatchEntryPanel : public QWidget {
    Q_OBJECT
public:
    WatchEntryPanel(QWidget *parent = nullptr);
    ~WatchEntryPanel();

    void setModel(QAbstractItemModel *);

private:
    Ui::WatchEntryPanel *ui;
};
