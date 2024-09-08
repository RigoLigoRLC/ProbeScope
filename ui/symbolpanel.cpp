
#include "symbolpanel.h"
#include "expressionevaluator.h"
#include "symbolbackend.h"
#include "ui_symbolpanel.h"
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QTreeWidget>



SymbolPanel::SymbolPanel(QWidget *parent) : QWidget(parent) {
    ui = new Ui::SymbolPanel;
    ui->setupUi(this);

    // Set props for sorter column
    ui->treeSymbolTree->sortByColumn(SortCol, Qt::AscendingOrder);
    ui->treeSymbolTree->setColumnHidden(SortCol, true);

    // Initialize rich text item delegate
    m_htmlDelegate = new HTMLDelegate(this);
    ui->treeSymbolTree->setItemDelegateForColumn(GeneralCol, m_htmlDelegate);

    connect(ui->treeSymbolTree, &QTreeWidget::itemExpanded, this, &SymbolPanel::sltItemExpanded);
    connect(ui->btnAddWatchEntry, &QPushButton::clicked, this, &SymbolPanel::sltAddWatchEntryClicked);
    connect(ui->btnEvalExpr, &QPushButton::clicked, this, &SymbolPanel::sltTestEvalExprClicked);
}

SymbolPanel::~SymbolPanel() {
    delete ui;
}

void SymbolPanel::setSymbolBackend(SymbolBackend *backend) {
    m_symbolBackend = backend;
}

bool SymbolPanel::buildRootFiles(SymbolBackend *const symbolBackend) {
    auto result = symbolBackend->getSourceFileList();
    if (result.isErr()) {
        QMessageBox::critical(this, tr("Cannot build symbol tree"),
                              tr("Symbol backend error when building symbol tree:\n\n%1")
                                  .arg(SymbolBackend::errorString(result.unwrapErr())));
        return false;
    }

    size_t index = 0;
    foreach (auto src, result.unwrap()) {
        auto *fileItem = new QTreeWidgetItem;
        fileItem->setText(GeneralCol, src.path);
        fileItem->setIcon(GeneralCol, QIcon(":/icons/blank_file.svg"));
        fileItem->setData(GeneralCol, NodeKindRole, uint32_t(NodeKind::CompileUnit));
        fileItem->setData(GeneralCol, CompileUnitIndexRole, src.cuIndex);
        fileItem->setText(SortCol, QString::number(index++));
        markNeedsPopulate(fileItem);
        ui->treeSymbolTree->addTopLevelItem(fileItem);
    }

    return true;
}

QTreeWidgetItem *SymbolPanel::buildSymbolItem(SymbolBackend *const symbolBackend,
                                              SymbolBackend::VariableInfo variable) {
    auto item = new QTreeWidgetItem;
    item->setText(0, variable.name);

    return item;
}

void SymbolPanel::markNeedsPopulate(QTreeWidgetItem *item) {
    item->setData(0, NeedsPopulateChildrenRole, true);

    auto loadingItem = new QTreeWidgetItem();
    loadingItem->setText(0, tr("Loading..."));
    item->addChild(loadingItem);
}

void SymbolPanel::sltItemExpanded(QTreeWidgetItem *item) {
    if (item->data(0, NeedsPopulateChildrenRole).toBool()) {
        switch (NodeKind(item->data(0, NodeKindRole).toUInt())) {
            case NodeKind::CompileUnit:
                dynamicPopulateChildForCU(item);
                break;
            case NodeKind::VariableEntries:
                dynamicPopulateChildForVarnode(item);
                break;
        }
    }
}

void SymbolPanel::sltAddWatchEntryClicked() {
    auto selected = ui->treeSymbolTree->selectedItems();
    if (selected.isEmpty() || selected.size() > 1) {
        return;
    }

    // Traverse to the topmost parent item
    QList<QTreeWidgetItem *> nodeChain;
    auto currentItem = selected[0];
    while (currentItem && NodeKind(currentItem->data(GeneralCol, NodeKindRole).toUInt()) != NodeKind::CompileUnit) {
        nodeChain.append(currentItem);
        currentItem = currentItem->parent();
    }
    Q_ASSERT(nodeChain.size() >= 1);

    QString expr = nodeChain.last()->data(GeneralCol, VariableNameRole).toString();
    // Now the node chain is formed, index 0 is innermost node, we traverse from the outmost element
    // Note that the outmost node would be put into expr directly, before the loop starts
    // Inside the loop we process the relationship between current and parent node, with knowledge of inner node
    if (nodeChain.size() >= 2) {
        for (int i = nodeChain.size() - 2; i >= 0; i--) {
            QTreeWidgetItem *current = nodeChain[i], *parent = nodeChain[i + 1], *inner = nullptr;
            if (i) {
                inner = nodeChain[i - 1];
            }


            switch (SymbolBackend::VariableIconType(parent->data(GeneralCol, IconTypeRole).toUInt())) {
                case SymbolBackend::VariableIconType::Integer:
                case SymbolBackend::VariableIconType::FloatingPoint:
                case SymbolBackend::VariableIconType::Boolean:
                    Q_ASSERT(false);
                    break;
                case SymbolBackend::VariableIconType::Structure:
                    // Parent = struct/union/class, Current = member
                    expr = QString("%1.%2").arg(expr).arg(current->data(GeneralCol, VariableNameRole).toString());
                    break;
                case SymbolBackend::VariableIconType::Pointer:
                    // Parent = ptr, Current = deref'd
                    // Some percise logics:
                    // If current = ptr (parent is high order pointer), don't use parenthesis;
                    // If inner is null (current is already deepest), don't use parenthesis
                    if (current->data(GeneralCol, IconTypeRole).toUInt() ==
                            uint32_t(SymbolBackend::VariableIconType::Pointer) ||
                        !inner) {
                        expr.prepend('*');
                    } else {
                        expr = QString("(*%1)").arg(expr);
                    }
                    continue;
                case SymbolBackend::VariableIconType::Array:
                    // Parent = array/subrange, Current = index'd
                    expr = QString("%1[%2]").arg(expr).arg(current->text(SortCol));
                    break;
                case SymbolBackend::VariableIconType::Unknown:
                    Q_ASSERT(false);
                    break;
            }
        }
    }


    QMessageBox::information(this, "Expression generation", expr);
    ExpressionEvaluator evalr;
    auto parseResult = evalr.parseToTokens(expr);
    qInfo() << expr << "Evaluation:";
    if (parseResult.isErr()) {
        qCritical() << parseResult.unwrapErr();
    } else {
        auto tokens = parseResult.unwrap();
        foreach (auto &i, tokens) {
            qInfo() << ExpressionEvaluator::tokenTypeToString(i.type) << i.embedData;
        }
    }
}

void SymbolPanel::sltTestEvalExprClicked() {
    auto expr = QInputDialog::getText(this, "Expr", "Put expr");
    auto parseResult = ExpressionEvaluator::treeSitter(expr);
    if (parseResult.isErr()) {
        QMessageBox::critical(this, "Error parsing expression", expr + '\n' + parseResult.unwrapErr());
    } else {
        // QString result;
        // auto tokens = parseResult.unwrap();
        // foreach (auto &i, tokens) {
        //     result += QString("%1: [%2]\n").arg(ExpressionEvaluator::tokenTypeToString(i.type),
        //     i.embedData.toString());
        // }
        QString result = parseResult.unwrap();
        QMessageBox::information(this, "Parse result", expr + '\n' + result);
    }
}

void SymbolPanel::dynamicPopulateChildForCU(QTreeWidgetItem *item) {
    auto cuIndex = item->data(0, CompileUnitIndexRole).toUInt();

    auto result = m_symbolBackend->getVariableOfSourceFile(cuIndex);
    if (result.isErr()) {
        return;
    }

    // Delete children
    auto childCount = item->childCount();
    for (int i = 0; i < childCount; i++) {
        auto child = item->takeChild(0);
        delete child;
    }

    // Insert new variables
    auto varList = result.unwrap();
    size_t index = 0;
    foreach (auto &var, varList) {
        insertNodeByVarnodeInfo(var, item, cuIndex, index++);
    }
}

void SymbolPanel::dynamicPopulateChildForVarnode(QTreeWidgetItem *item) {
    auto varName = item->data(0, VariableNameRole).toString();
    auto cuIndex = item->data(0, CompileUnitIndexRole).toUInt();
    auto typespec = item->data(0, TypespecRole).value<SymbolBackend::TypeDieRef>();

    auto result = m_symbolBackend->getVariableChildren(varName, cuIndex, typespec);
    if (result.isErr()) {
        // TODO:
        qCritical() << "Failed to expand Varnode" << varName;
        return;
    }

    // Delete children
    auto childCount = item->childCount();
    for (int i = 0; i < childCount; i++) {
        auto child = item->takeChild(0);
        delete child;
    }

    // Insert new variables
    auto varList = result.unwrap().subNodeDetails;
    size_t index = 0;
    foreach (auto &var, varList) {
        insertNodeByVarnodeInfo(var, item, cuIndex, index++);
    }
}

void SymbolPanel::insertNodeByVarnodeInfo(SymbolBackend::VariableNode var, QTreeWidgetItem *parent, uint32_t cuIndex,
                                          size_t sortColIdx) {
    auto *subitem = new QTreeWidgetItem;
    auto escapedDisplayTypeName = var.displayTypeName;
    escapedDisplayTypeName.replace('<', "&lt;").replace('>', "&gt;");
    subitem->setText(0, QString("<html><span>"
                                "%1 <span style=\"font-style:italic;color:#9f9f9f;\">(%2)<span/>"
                                "<span/><html/>")
                            .arg(var.displayName, escapedDisplayTypeName));
    subitem->setData(0, VariableNameRole, var.displayName);
    subitem->setData(0, CompileUnitIndexRole, cuIndex);
    subitem->setData(0, TypespecRole, var.typeSpec);
    subitem->setData(0, NodeKindRole, uint32_t(NodeKind::VariableEntries));
    subitem->setData(GeneralCol, IconTypeRole, uint32_t(var.iconType));
    subitem->setText(SortCol, QString::number(sortColIdx));
    if (var.expandable) {
        markNeedsPopulate(subitem);
    }
    switch (var.iconType) {
        case SymbolBackend::VariableIconType::Boolean:
        case SymbolBackend::VariableIconType::Integer:
            subitem->setIcon(0, QIcon(":/icons/trivial_variable_integer.svg"));
            break;
        case SymbolBackend::VariableIconType::FloatingPoint:
            subitem->setIcon(0, QIcon(":/icons/trivial_variable_floating_point.svg"));
            break;
        case SymbolBackend::VariableIconType::Structure:
            subitem->setIcon(0, QIcon(":/icons/structure.svg"));
            break;
        case SymbolBackend::VariableIconType::Pointer:
            subitem->setIcon(0, QIcon(":/icons/trivial_variable_trivial_pointer.svg"));
            break;
        case SymbolBackend::VariableIconType::Array:
            subitem->setIcon(0, QIcon(":/icons/trivial_variable_array.svg"));
            break;
        case SymbolBackend::VariableIconType::Unknown:
            subitem->setIcon(0, QIcon(":/icons/trivial_variable_unsupported.svg"));
            break;
    }

    parent->addChild(subitem);
}
