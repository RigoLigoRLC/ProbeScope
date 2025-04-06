
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
    // TODO: do the things
    return QVariant();
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
