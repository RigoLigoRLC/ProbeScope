
#pragma once

#include "serialization/workspace.h"
#include <QAbstractTableModel>
#include <QVector>

class WorkspaceModel;

class WatchEntryModel : public QAbstractTableModel {
    Q_OBJECT
public:
    WatchEntryModel(WorkspaceModel *workspaceModel, QObject *parent);
    virtual ~WatchEntryModel() {}

    enum Columns {
        Color,
        DisplayName,
        Expression,
        PlotAreas,
        Thickness,
        LineStyle,
        FrequencyLimit,

        MaxColumns,
        FrequencyFeedback,
        ExpressionOkay,
    };

    //
    // Reimplemented functions
    //
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // This one seems useless in our case
    // virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    void addRowForEntry(size_t entryId);
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    //
    // Interface functions
    //

    /**
     * @brief Call this function when the UI display for an entry has to be invalidated but the data change was not made
     * from the setData function.
     * @param entryId Watch entry ID
     * @param prop Which property of the entry has changed
     */
    void invalidateEntryDataDisplay(size_t entryId, Columns prop);

    /**
     * @brief When workspace receives a frequency feedback, this function is called to notify the UI to refresh
     * acquisition frequency display.
     * @param entryId Watch entry ID.
     */
    void notifyFrequencyFeedbackChanged(size_t entryId);

    /**
     * @brief This variant is called when acquisition starts or stops so that all the entries may switch between
     * frequency limit and actual feedback.
     */
    void notifyFrequencyFeedbackChanged();

    /**
     * @brief Called internally by WorkspaceModel to remove an entry from inside of the model
     * @param entryId Watch entry ID.
     */
    void removeEntry(size_t entryId);

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
