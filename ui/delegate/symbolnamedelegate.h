
// Inspired by https://stackoverflow.com/questions/1956542/how-to-make-item-view-render-rich-html-text-in-qt

#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextDocument>

class SymbolNameDelegate : public QStyledItemDelegate {
public:
    SymbolNameDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent){};

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    static constexpr int m_heightMargin = 2;
    static constexpr int m_typeMargin = 5;
};
