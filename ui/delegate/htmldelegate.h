
// Taken from https://stackoverflow.com/questions/1956542/how-to-make-item-view-render-rich-html-text-in-qt

#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <qstyleditemdelegate.h>

class HTMLDelegate : public QStyledItemDelegate {
public:
    HTMLDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent){};

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    int m_height = 12;
};
