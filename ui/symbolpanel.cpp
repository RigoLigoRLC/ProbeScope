
#include "symbolpanel.h"
#include "delegate/symbolnamedelegate.h"
#include "eventfilters/firstcolumnfollowresizefilter.h"
#include "expressionevaluator/optimizer.h"
#include "expressionevaluator/parser.h"
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
    ui->treeSymbolTree->header()->resizeSection(GeneralCol, 300);
    ui->treeSymbolTree->header()->resizeSection(AddressCol, 150);
    // ui->treeSymbolTree->header()->installEventFilter(new FirstColumnFollowResizeFilter(this));

    // Initialize rich text item delegate
    m_symbolNameDelegate = new SymbolNameDelegate(this);
    ui->treeSymbolTree->setItemDelegateForColumn(GeneralCol, m_symbolNameDelegate);

    connect(ui->treeSymbolTree, &QTreeWidget::itemExpanded, this, &SymbolPanel::sltItemExpanded);
    connect(ui->btnAddWatchEntry, &QPushButton::clicked, this, &SymbolPanel::sltAddWatchEntryClicked);
    connect(ui->btnEvalExpr, &QPushButton::clicked, this, &SymbolPanel::sltTestEvalExprClicked);
    connect(ui->btnVarStore, &QPushButton::clicked, this, &SymbolPanel::sltTestVarStoreClicked);
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
        fileItem->setText(GeneralCol, src);
        fileItem->setIcon(GeneralCol, QIcon::fromTheme("variablepanel-blank-file"));
        fileItem->setData(GeneralCol, NodeKindRole, uint32_t(NodeKind::CompileUnit));
        // fileItem->setData(GeneralCol, CompileUnitIndexRole, src.cuIndex);
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
            case NodeKind::CompileUnit: dynamicPopulateChildForCU(item); break;
            case NodeKind::VariableEntries: dynamicPopulateChildForVarnode(item); break;
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
                case SymbolBackend::VariableIconType::Boolean: Q_ASSERT(false); break;
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
                case SymbolBackend::VariableIconType::Unknown: Q_ASSERT(false); break;
            }
        }
    }


    QMessageBox::information(this, "Expression generation", expr);
    auto parseResult = ExpressionEvaluator::Parser::parseToBytecode(expr);
    qInfo() << expr << "Evaluation:";
    if (parseResult.isErr()) {
        qCritical() << parseResult.unwrapErr();
    } else {
        auto bytecode = parseResult.unwrap();
        qInfo().noquote() << bytecode.disassemble();
        qInfo() << "Optimization:";
        auto optimizationResult = ExpressionEvaluator::StaticOptimize(bytecode, m_symbolBackend);
        if (optimizationResult.isErr()) {
            qInfo() << "Error:" << optimizationResult.unwrapErr();
        } else {
            auto optimizedBytecode = optimizationResult.unwrap();
            qInfo().noquote() << optimizedBytecode.disassemble();
        }
    }

    emit addWatchExpression(expr);
}

void SymbolPanel::sltTestEvalExprClicked() {
    auto expr = QInputDialog::getText(this, "Expr", "Put expr");
    auto parseResult = ExpressionEvaluator::Parser::treeSitter(expr);
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

void SymbolPanel::sltTestVarStoreClicked() {
    auto selected = ui->treeSymbolTree->selectedItems();
    if (selected.isEmpty() || selected.size() > 1) {
        return;
    }

    auto item = selected[0];
    auto varName = item->data(0, VariableNameRole).toString();
    QString queryLog;
    if (varName.contains("::")) {
        // Split to scoped queries
        auto parts = varName.split("::");
        auto scope = m_symbolBackend->getRootScope();
        int i;
        for (i = 0; i < parts.size() - 1; ++i) {
            scope = scope->getSubScope(parts[i]);
            queryLog += QStringLiteral("-> %1: %2\n").arg(parts[i]).arg(quintptr(scope.get()));
            if (!scope) {
                queryLog += "FAIL";
                break;
            }
        }
        if (scope) {
            auto var = scope->getVariable(parts[i]);
            queryLog += QStringLiteral("-> %1: %2\n").arg(parts[i]).arg(quintptr(var.get()));
            if (var) {
                queryLog += "PASS";
            } else {
                queryLog += "FAIL";
            }
        }
    } else {
        // Query in root namespace
        auto var = m_symbolBackend->getRootScope()->getVariable(varName);
        if (var) {
            queryLog += "PASS";
        } else {
            queryLog += "FAIL";
        }
    }
    QMessageBox::information(this, "Query result", queryLog);
}

void SymbolPanel::dynamicPopulateChildForCU(QTreeWidgetItem *item) {
    // auto cuIndex = item->data(0, CompileUnitIndexRole).toUInt();

    auto result = m_symbolBackend->getVariableOfSourceFile(item->text(GeneralCol));
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
        insertNodeByVarnodeInfo(var, item, index++);
    }
}

void SymbolPanel::dynamicPopulateChildForVarnode(QTreeWidgetItem *item) {
    auto varName = item->data(0, VariableNameRole).toString();
    auto typeObj = item->data(0, TypeObjectRole).value<ITypePtrBox>().p;
    auto location = item->data(0, VariableLocationRole).value<VariableLocationDesc>();

    auto result = m_symbolBackend->getVariableChildren(location.byteOffset, typeObj);
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
        insertNodeByVarnodeInfo(var, item, index++);
    }
}

void SymbolPanel::insertNodeByVarnodeInfo(SymbolBackend::VariableNode var, QTreeWidgetItem *parent, size_t sortColIdx) {
    auto *subitem = new QTreeWidgetItem;
    QString addressText;
    if (var.address.has_value()) {
        char buf[19];
        int n = snprintf(buf, sizeof(buf), "0x%08llX", var.address.value()); // Good old C formatter is far better
        if (n > sizeof(buf)) {
            addressText = "0x" + QString::number(var.address.value(), 16); // Some one pushed it beyond limits...
        } else {
            addressText = buf;
        }
        if (var.bitSize) { // Bitfield occupation
            addressText += QString(" [%2:%1]").arg(var.bitOffset).arg(var.bitSize + var.bitOffset - 1);
        }
    } else {
        addressText = "*";
    }
    subitem->setText(0, var.displayName);
    subitem->setData(0, VariableNameRole, var.displayName);
    subitem->setData(0, VariableLocationRole, VariableLocationDesc{var.address, var.bitOffset, var.bitSize});
    subitem->setData(0, TypeNameRole, var.typeObj->fullyQualifiedName());
    subitem->setData(0, TypeObjectRole, QVariant::fromValue(ITypePtrBox{var.typeObj}));
    subitem->setData(0, NodeKindRole, uint32_t(NodeKind::VariableEntries));
    subitem->setData(GeneralCol, IconTypeRole, uint32_t(var.iconType));
    subitem->setText(AddressCol, addressText);
    subitem->setText(ByteSizeCol, QString::number(var.typeObj->getSizeof()));
    subitem->setText(SortCol, QString::number(sortColIdx));
    if (var.expandable) {
        markNeedsPopulate(subitem);
    }
    switch (var.iconType) {
        case SymbolBackend::VariableIconType::Boolean:
        case SymbolBackend::VariableIconType::Integer:
            subitem->setIcon(0, QIcon::fromTheme("variablepanel-integer"));
            break;
        case SymbolBackend::VariableIconType::FloatingPoint:
            subitem->setIcon(0, QIcon::fromTheme("variablepanel-floating-point"));
            break;
        case SymbolBackend::VariableIconType::Structure:
            subitem->setIcon(0, QIcon::fromTheme("variablepanel-structure"));
            break;
        case SymbolBackend::VariableIconType::Pointer:
            subitem->setIcon(0, QIcon::fromTheme("variablepanel-pointer"));
            break;
        case SymbolBackend::VariableIconType::Array:
            subitem->setIcon(0, QIcon::fromTheme("variablepanel-array"));
            break;
        case SymbolBackend::VariableIconType::Unknown:
            subitem->setIcon(0, QIcon::fromTheme("variablepanel-unsupported"));
            break;
    }

    parent->addChild(subitem);
}
