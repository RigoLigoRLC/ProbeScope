
#include "result.h"
#include "uistatebridge.h"
#include <QPainter>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

class PlotAreaColumnDelegate : public QStyledItemDelegate {
public:
    PlotAreaColumnDelegate(IUiStateBridge *uiBridge, QObject *parent = nullptr);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const override;
    virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

protected slots:
    void commitAndCloseEditor();

private:
    Result<QString, QString> strFromIndex(const QModelIndex &index) const;

private:
    IUiStateBridge *m_uiBridge;
    QStandardItemModel *m_checkBoxModel;
    static constexpr int m_heightMargin = 2;
    static constexpr int m_horizontalMargin = 5;
};
