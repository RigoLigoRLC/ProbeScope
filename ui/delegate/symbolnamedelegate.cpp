
#include "symbolnamedelegate.h"
#include "symbolpanel.h"
#include <QDebug>

void SymbolNameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    painter->save();

    // Draw split line
    painter->setPen(options.palette.color(QPalette::Midlight));
    painter->drawLine(options.rect.topRight(), options.rect.bottomRight());

    options.text = "";
#ifdef Q_OS_WIN
    // Qt5 internally gives icons a 2 pixel padding by padding the iconRect (qwindowsvistastyle.cpp L2021)
    // But when QIcon draws Scalable Icons they fucked it up and forgot to preverse aspect ratio
    // We work around it by eliminating the padding
    options.decorationSize.rwidth() -= 4;
#endif
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, option.widget);

    // shift text right to make icon visible
    constexpr int hPadding = 4;
    QSize iconSize = options.icon.actualSize(options.decorationSize);
    QRect clip(0, 0, options.rect.width() - iconSize.width() - hPadding, options.rect.height());
    painter->translate(options.rect.left() + iconSize.width() + hPadding, options.rect.top());

    painter->setClipRect(clip);

    // Draw name
    auto font = options.font;
    auto fm = painter->fontMetrics();
    auto mainText = index.data(Qt::DisplayRole).toString();
    painter->setFont(font);
    painter->setPen(options.palette.color(QPalette::Text));
    painter->drawText(clip.adjusted(0, m_heightMargin, 0, 0), Qt::TextSingleLine, mainText);

    // If there is a type name, draw it
    if (index.data(SymbolPanel::TypeNameRole).isValid()) {
        QString typeName = index.data(SymbolPanel::TypeNameRole).toString();
        font.setItalic(true);
        painter->setFont(font);
        painter->setPen(options.palette.color(QPalette::Disabled, QPalette::Text));
        painter->drawText(clip.adjusted(fm.horizontalAdvance(mainText) + m_typeMargin, m_heightMargin, 0, 0),
                          Qt::TextSingleLine, typeName);
    }

    painter->restore();
}

QSize SymbolNameDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    QFontMetrics fm(options.font);
    return QSize(fm.horizontalAdvance(options.text), fm.height() + m_heightMargin * 2);
}
