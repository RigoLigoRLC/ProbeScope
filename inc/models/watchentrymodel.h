
#pragma once

#include "serialization/workspace.h"
#include <QAbstractTableModel>
#include <QVector>

class WorkspaceModel;

class WatchEntryModel : public QAbstractTableModel {
    Q_OBJECT
public:
    WatchEntryModel(WorkspaceModel *workspaceModel, QObject *parent)
        : QAbstractTableModel(parent), m_workspace(workspaceModel) {}
    virtual ~WatchEntryModel() {}

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // This one seems useless in our case
    // virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    void addRowForEntry(size_t entryId);
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    enum Columns {
        Color,
        DisplayName,
        Expression,
        PlotAreas,
        Thickness,
        LineStyle,
    };

private:
    /**
     * @brief Returns the row number on which the user can double-click to add a watch expression manually on the UI.
     */
    int rowForManualAppend() const { return m_watchEntryIds.size(); };

private:
    WorkspaceModel *m_workspace;

    /**
     * @brief The index into this vector is the row number, the value is the internal watch entry ID at this row.
     */
    QVector<size_t> m_watchEntryIds;
};
