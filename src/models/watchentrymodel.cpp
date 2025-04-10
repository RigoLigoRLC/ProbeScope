
#include "models/watchentrymodel.h"
#include "workspacemodel.h"

int WatchEntryModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : 6;
};

int WatchEntryModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_watchEntryIds.size() + 1;
}

QVariant WatchEntryModel::data(const QModelIndex &index, int role) const {
    if (index.row() == rowForManualAppend()) {
        // The "Double click to add watch entry" row
        if (index.column() != Columns::Expression) {
            // Rows other than "Expression" are empty and non editable
            switch (role) {
                case Qt::EditRole: return false;
                default: return QVariant();
            }
        }
        switch (role) {
            case Qt::FontRole: {
                // Return a default font with italic set
                auto defFont = QFont();
                defFont.setItalic(true);
                return defFont;
            }
            case Qt::DisplayRole: return tr("Double click to add watch entry");
            default: return QVariant();
        }
    }

    // Actual rows with data
    // TODO: how to deal with these unwraps? In C++ you can't `let result = match index.column {`
    auto entry = m_watchEntryIds[index.row()];
    switch (index.column()) {
        case Color: {
            switch (role) {
                case Qt::DecorationRole: return m_workspace->getWatchEntryGraphProperty(entry, Color).unwrap();
            }
            break;
        }
        case DisplayName: {
            switch (role) {
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, DisplayName).unwrap();
            }
            break;
        }
        case Expression: {
            switch (role) {
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, Expression).unwrap();
            }
            break;
        }
        case PlotAreas: {
            switch (role) {
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, PlotAreas).unwrap();
            }
            break;
        }
        case Thickness: {
            switch (role) {
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, Thickness).unwrap();
            }
            break;
        }
        case LineStyle: {
            switch (role) {
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, LineStyle).unwrap();
            }
            break;
        }
    }
    return QVariant();
}

void WatchEntryModel::addRowForEntry(size_t entryId) {
    emit beginInsertRows(QModelIndex(), rowCount(), rowCount());

    m_watchEntryIds.append(entryId);

    emit endInsertRows();
}

bool WatchEntryModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (row < 0 || row > m_watchEntryIds.size() || row + count > m_watchEntryIds.size()) {
        return false;
    }

    emit beginRemoveRows(parent, row, row + count);

    auto from = m_watchEntryIds.begin() + row;
    auto to = from + count;
    for (auto it = from; it != to; ++it) {
        if (auto result = m_workspace->removeWatchEntry(*it); result.isErr()) {
            qCritical() << "Remove watch entry" << *it << "fail:" << result.unwrapErr();
        }
    }
    m_watchEntryIds.erase(from, to);

    emit endRemoveRows();
    return true;
}
