
#include "plotareacolumndelegate.h"
#include "workspacemodel.h"
#include <QComboBox>
#include <ranges>

using namespace Qt::StringLiterals;

//
// Helper model class to display a checkable combobox
//
class PlotAreaSelectorModel : public QStandardItemModel {
    Q_OBJECT
public:
    PlotAreaSelectorModel(QObject *parent = nullptr) : QStandardItemModel(parent) {}
    static constexpr auto PlotAreaIdRole = Qt::UserRole + 1;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override {
        return QStandardItemModel::flags(index) | Qt::ItemIsUserCheckable;
    }
};

//
// Helper combobox that knows when the popup disappears
//
class PlotAreaSelectorBox : public QComboBox {
    Q_OBJECT
public:
    PlotAreaSelectorBox(QWidget *parent = nullptr) : QComboBox(parent) {}

    virtual void hidePopup() override {
        QComboBox::hidePopup();
        emit editingCompleted();
    }

signals:
    void editingCompleted();
};

//
// Helper list view that knows when the popup disappears
//
class PlotAreaSelectorView : public QListView {
    Q_OBJECT
public:
    PlotAreaSelectorView(QWidget *parent = nullptr) : QListView(parent) {}

protected:
    virtual void focusOutEvent(QFocusEvent *event) override {
        QListView::focusOutEvent(event);
        emit editingCompleted();
    }

signals:
    void editingCompleted();
};

PlotAreaColumnDelegate::PlotAreaColumnDelegate(IUiStateBridge *uiBridge, QObject *parent)
    : m_uiBridge(uiBridge), QStyledItemDelegate(parent) {
    //
    m_checkBoxModel = new PlotAreaSelectorModel(this);
};

void PlotAreaColumnDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const {
    QStyleOptionViewItem options = option;

#if 0
    initStyleOption(&options, index);

    painter->save();

    // Draw split line
    painter->setPen(options.palette.color(QPalette::Midlight));
    painter->drawLine(options.rect.topRight(), options.rect.bottomRight());

    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, option.widget);

    painter->setClipRect(options.rect);

    // Draw text
    auto textResult = strFromIndex(index);
    QString dispText = textResult.isErr() ? textResult.unwrapErr() : textResult.unwrap();
    if (dispText.isEmpty()) {
        painter->restore();
        return;
    }

    auto drawRegion = options.rect;
    drawRegion.adjust()

    auto font = option.font;
    font.setItalic(textResult.isErr());
    painter->setFont(font);
    painter->setPen(options.palette.color(textResult.isErr() ? QPalette::Disabled : QPalette::Active, QPalette::Text));

    painter->drawText(options.rect, Qt::TextSingleLine, dispText);

    painter->restore();


#endif
    auto textResult = strFromIndex(index);
    options.text = textResult.isErr() ? textResult.unwrapErr() : textResult.unwrap();
    options.font.setItalic(textResult.isErr());
    options.palette.setColor(
        QPalette::Text,
        options.palette.color(textResult.isErr() ? QPalette::Disabled : QPalette::Active, QPalette::Text));

    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, option.widget);
}

QSize PlotAreaColumnDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    //
    return {};
}

Result<QString, QString> PlotAreaColumnDelegate::strFromIndex(const QModelIndex &index) const {
    //
    if (int(index.data().typeId()) != QMetaTypeId<WorkspaceModel::PlotAreas>::qt_metatype_id()) {
        return Err(u""_s);
    }

    auto &&idSet = index.data().value<WorkspaceModel::PlotAreas>();
    if (idSet.isEmpty()) {
        return Err(tr("(Unassigned)"));
    }

    if (idSet.size() == 1) {
        return Ok(m_uiBridge->getPlotAreaName(*idSet.begin()));
    }

    return Ok(tr("(%1 areas)", "%1 is the count of areas that a plot is assigned to").arg(idSet.size()));
}

void PlotAreaColumnDelegate::commitAndCloseEditor() {
    auto *editor = qobject_cast<PlotAreaSelectorBox *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

QWidget *PlotAreaColumnDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                              const QModelIndex &index) const {
#if 0
    // Use a combobox to reuse Qt's popup code
    auto combo = new PlotAreaSelectorBox(parent);
    combo->view()->setModel(m_checkBoxModel);
    connect(combo, &PlotAreaSelectorBox::editingCompleted, this, &PlotAreaColumnDelegate::commitAndCloseEditor);
    combo->showPopup();
    return combo;
#else
    auto listView = new PlotAreaSelectorView();
    listView->setWindowFlag(Qt::WindowCloseButtonHint, false);
    listView->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    listView->setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    // listView->setWindowFlag(Qt::Tool, true);
    listView->setModel(m_checkBoxModel);
    listView->setWindowTitle(tr("Select Plotting Areas..."));
    connect(listView, &PlotAreaSelectorView::editingCompleted, this, &PlotAreaColumnDelegate::commitAndCloseEditor);
    return listView;
#endif
}

void PlotAreaColumnDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                                  const QModelIndex &index) const {
    // Move the editor a bit closer to the cell
    auto geom = editor->geometry();
    auto bottomLeft = option.widget->mapToGlobal(option.rect.bottomLeft());
    geom.setWidth(200);
    geom.setHeight(100);
    geom.moveBottomLeft(bottomLeft);
    editor->setGeometry(geom);
}

void PlotAreaColumnDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    // Update checkbox model's data
    m_checkBoxModel->clear();
    auto idSet = index.data().value<WorkspaceModel::PlotAreas>();
    qDebug() << "setEditorData: ID set" << idSet;
    for (auto [k, v] : m_uiBridge->collectPlotAreaNames().asKeyValueRange()) {
        auto item = new QStandardItem(v);
        auto checkState = idSet.contains(k) ? Qt::Checked : Qt::Unchecked;
        item->setData(checkState, Qt::CheckStateRole);
        qDebug() << "Checkstate: Area=" << k << ",State=" << checkState;
        item->setData(k, PlotAreaSelectorModel::PlotAreaIdRole);
        m_checkBoxModel->appendRow(item);
    };
}

void PlotAreaColumnDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    //
    WorkspaceModel::PlotAreas idSet;
    for (int i = 0; i < m_checkBoxModel->rowCount(); ++i) {
        auto item = m_checkBoxModel->item(i);
        if (item->data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked) {
            idSet.insert(item->data(PlotAreaSelectorModel::PlotAreaIdRole).toULongLong());
        }
    }
    model->setData(index, QVariant::fromValue(idSet));
}

#include "plotareacolumndelegate.moc"
