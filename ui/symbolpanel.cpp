
#include "symbolpanel.h"
#include "symbolbackend.h"
#include "ui_symbolpanel.h"
#include <QMessageBox>
#include <qtreewidget.h>

SymbolPanel::SymbolPanel(QWidget *parent) : QWidget(parent) {
    ui = new Ui::SymbolPanel;
    ui->setupUi(this);

    connect(ui->treeSymbolTree, &QTreeWidget::itemExpanded, this, &SymbolPanel::sltItemExpanded);
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

    foreach (auto src, result.unwrap()) {
        auto *fileItem = new QTreeWidgetItem;
        fileItem->setText(0, src.path);
        fileItem->setIcon(0, QIcon(":/icons/blank_file.svg"));
        fileItem->setData(0, NodeKindRole, uint32_t(NodeKind::CompileUnit));
        fileItem->setData(0, CompileUnitIndexRole, src.cuIndex);
        markNeedsPopulate(fileItem);
        ui->treeSymbolTree->addTopLevelItem(fileItem);
    }

    return true;
}

QTreeWidgetItem *SymbolPanel::buildSymbolItem(SymbolBackend *const symbolBackend,
                                              SymbolBackend::VariableInfo variable) {
    auto item = new QTreeWidgetItem;
    item->setText(0, variable.name);
    // item->setData(0, TrivialTypeKind, int(SymbolTrivialType::Unsupported));

    if (variable.childrenCount == 0) {
        // Assume it is a trivial type, try take its type
        auto trivialType = explainedTypeToTrivialType(variable.explainedType);
        // item->setData(0, TrivialTypeKind, int(trivialType));

        // Set expression
        // item->setData(0, Expression, variable.expression);

        // Set icon accordingly
        switch (trivialType) {
            case SymbolTrivialType::Uint8:
            case SymbolTrivialType::Sint8:
            case SymbolTrivialType::Uint16:
            case SymbolTrivialType::Sint16:
            case SymbolTrivialType::Uint32:
            case SymbolTrivialType::Sint32:
            case SymbolTrivialType::Uint64:
            case SymbolTrivialType::Sint64:
                item->setIcon(0, QIcon(":/icons/trivial_variable_integer.svg"));
                break;
            case SymbolTrivialType::Float32:
            case SymbolTrivialType::Float64:
                item->setIcon(0, QIcon(":/icons/trivial_variable_floating_point.svg"));
                break;
            case SymbolTrivialType::Unsupported:
                item->setIcon(0, QIcon(":/icons/trivial_variable_unsupported.svg"));
                break;
        }

        return item;
    } else {
        // Assume it is at least a structure or array

        if (variable.typeName.contains(']')) {
            // Has a bracket ending, assume it is an array
            item->setIcon(0, QIcon(":/icons/trivial_variable_array.svg"));
        }

        // Only go through children when it's not an array because dealing with large arrays is SO SLOW
        if (!variable.typeName.contains(']')) {
            auto children = symbolBackend->getVariableChildren(variable.expression);
            if (children.isErr()) {
                item->setIcon(0, QIcon(":/icons/trivial_variable_unsupported.svg"));
                return item;
            }
            // Process all children recursively
            foreach (auto child, children.unwrap()) {
                item->addChild(buildSymbolItem(symbolBackend, child));
            }
        }

        return item;
    }
}


SymbolPanel::SymbolTrivialType SymbolPanel::explainedTypeToTrivialType(QString explainedType) {
    // Specs taken from https://downloads.ti.com/docs/esd/SPNU151T/data-types-stdz0555922.html
    // This is a horrible hack just because GDB won't tell you how wide a variable is in any sensible way
    bool signedness = false, fp = false;
    int length = 0;

    // FIXME: I would not apologize for any other MCUs that has 16 or 64 bit word width.
    // I only have ARM based ones and that's what I develop with.
    // GDB won't tell me the width of a variable properly so haters please just fuck with GNU directly.
    // GDB is responsible, not me.

    if (explainedType.startsWith("uint") || explainedType.startsWith("unsigned")) {
        signedness = true;
    }

    if (explainedType.contains("long long") || explainedType.endsWith("int64_t")) {
        length = 8;
    } else if (explainedType.contains("long") || explainedType.endsWith("int32_t")) {
        length = 4;
    } else if (explainedType.contains("short") || explainedType.endsWith("int16_t")) {
        length = 2;
    } else if (explainedType.contains("char") || explainedType.endsWith("int8_t")) {
        length = 1;
    } else if (explainedType.contains("int")) {
        length = 4;
    } else if (explainedType.contains("float")) {
        length = 4;
        fp = true;
    } else if (explainedType.contains("double") && !explainedType.contains("long double")) {
        length = 8;
        fp = true;
    }

    switch (length) {
        case 1:
            return (signedness ? SymbolTrivialType::Sint8 : SymbolTrivialType::Uint8);
        case 2:
            return (signedness ? SymbolTrivialType::Sint16 : SymbolTrivialType::Uint16);
        case 4:
            if (fp) {
                return SymbolTrivialType::Float32;
            } else {
                return (signedness ? SymbolTrivialType::Sint32 : SymbolTrivialType::Uint32);
            }
        case 8:
            return (signedness ? SymbolTrivialType::Sint64 : SymbolTrivialType::Uint64);
        default:
            return SymbolTrivialType::Unsupported;
    }
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
                break;
        }
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
    foreach (auto &var, varList) {
        auto *subitem = new QTreeWidgetItem;
        subitem->setText(0, QString("%1 (%2)").arg(var.displayName, var.displayTypeName));
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

        item->addChild(subitem);
    }
}
