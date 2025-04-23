
#pragma once

#include "delegate/plotareacolumndelegate.h"
#include "ui_watchentrypanel.h"
#include "uistatebridge.h"
#include <QWidget>

class QAbstractItemModel;
class WatchEntryPanel : public QWidget {
    Q_OBJECT
public:
    WatchEntryPanel(IUiStateBridge *uiBridge, QWidget *parent = nullptr);
    ~WatchEntryPanel();

    void setModel(QAbstractItemModel *);

private slots:
    void sltTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void sltBtnRemoveEntryClicked();

private:
    Ui::WatchEntryPanel *ui;
    IUiStateBridge *m_uiBridge;

    PlotAreaColumnDelegate *m_plotAreaColumnDelegate;
    QSet<int> m_selectedRows;
};
