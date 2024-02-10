
#pragma once

#include "symbolbackend.h"
#include "ui_symbolpanel.h"
#include <qtreewidget.h>

class SymbolPanel : public QWidget {
    Q_OBJECT
public:
    SymbolPanel(QWidget *parent = nullptr);
    ~SymbolPanel();

    enum SymbolItemIconType {
        SourceFile,
        Structure,
        Integer,
        FloatingPoint,
        Pointer,
    };

    enum class SymbolTrivialType {
        Uint8,
        Sint8,
        Uint16,
        Sint16,
        Uint32,
        Sint32,
        Uint64,
        Sint64,
        Float32,
        Float64,
        Unsupported,
    };

    enum class NodeKind {
        CompileUnit,
        VariableEntries,
    };

    enum SymbolItemDataRole { NeedsPopulateChildrenRole = Qt::UserRole + 1, NodeKindRole, CompileUnitIndexRole };

    void setSymbolBackend(SymbolBackend *);

    void buildRootSourceFileItems(QStringList files);
    bool buildRootFiles(SymbolBackend *const symbolBackend);
    QTreeWidgetItem *buildSymbolItem(SymbolBackend *const symbolBackend, SymbolBackend::VariableInfo variable);

    static SymbolTrivialType explainedTypeToTrivialType(QString explainedType);
    static void markNeedsPopulate(QTreeWidgetItem *item);

    Ui::SymbolPanel *ui;

private:
    SymbolBackend *m_symbolBackend;

private slots:
    void sltItemExpanded(QTreeWidgetItem *item);

private:
    void dynamicPopulateChildForCU(QTreeWidgetItem *item);
};
