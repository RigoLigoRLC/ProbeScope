
#include "models/watchentrymodel.h"
#include "workspacemodel.h"

using namespace Qt::Literals;

WatchEntryModel::WatchEntryModel(WorkspaceModel *workspaceModel, QObject *parent)
    : QAbstractTableModel(parent), m_workspace(workspaceModel) {
    //
}

int WatchEntryModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : MaxColumns;
};

int WatchEntryModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_watchEntryIds.size() + 1;
}

QVariant WatchEntryModel::data(const QModelIndex &index, int role) const {
    if (index.row() == rowForManualAppend()) {
        // The "Double click to add watch entry" row
        if (index.column() != Columns::Expression) {
            // Columns other than "Expression" are empty and non editable
            return {};
        }
        switch (role) {
            case Qt::FontRole: {
                // Return a default font with italic set
                auto defFont = QFont();
                defFont.setItalic(true);
                return defFont;
            }
            case Qt::DisplayRole: return tr("Double click to add watch entry");
            case Qt::EditRole: return u""_s;
            default: return QVariant();
        }
    }

    // Actual rows with data
    // TODO: how to deal with these unwraps? In C++ you can't `let result = match index.column {`
    auto entry = m_watchEntryIds[index.row()];
    switch (Columns(index.column())) {
        case Color: {
            switch (role) {
                case Qt::EditRole:
                case Qt::DecorationRole: return m_workspace->getWatchEntryGraphProperty(entry, Color).unwrap();
            }
            break;
        }
        case DisplayName: {
            switch (role) {
                case Qt::EditRole:
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, DisplayName).unwrap();
            }
            break;
        }
        case Expression: {
            switch (role) {
                case Qt::EditRole:
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, Expression).unwrap();
            }
            break;
        }
        case PlotAreas: {
            switch (role) {
                case Qt::EditRole:
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, PlotAreas).unwrap();
            }
            break;
        }
        case Thickness: {
            switch (role) {
                case Qt::EditRole:
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, Thickness).unwrap();
            }
            break;
        }
        case LineStyle: {
            switch (role) {
                case Qt::EditRole:
                case Qt::DisplayRole: return m_workspace->getWatchEntryGraphProperty(entry, LineStyle).unwrap();
            }
            break;
        }
        case FrequencyLimit: {
            switch (role) {
                case Qt::DisplayRole: {
                    // Frequency limit column is a bit different: to save space, this column shows frequency feedback
                    // when acquisition is live, and shows frequency limit when acquisition is not live.
                    if (m_workspace->isAcquisitionActive()) {
                        return m_workspace->getWatchEntryGraphProperty(entry, FrequencyFeedback).unwrap();
                    } else {
                        return m_workspace->getWatchEntryGraphProperty(entry, FrequencyLimit).unwrap();
                    }
                }
                case Qt::EditRole: return m_workspace->getWatchEntryGraphProperty(entry, FrequencyLimit).unwrap();
            }
        }

        // These are never shown as a column
        case MaxColumns:
        case FrequencyFeedback: return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags WatchEntryModel::flags(const QModelIndex &index) const {
    //
    if (index.row() == rowForManualAppend()) {
        // The "Double click to add watch entry" row
        if (index.column() != Columns::Expression) {
            // Columns other than "Expression" are empty and non editable
            return Qt::NoItemFlags;
        }
        return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    // Rows with data
    return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool WatchEntryModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (index.row() == rowForManualAppend()) {
        auto exprStr = value.toString();
        if (!exprStr.isEmpty()) {
            m_workspace->addWatchEntry(exprStr, {});
        }
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    auto setResult = [this, index, value]() {
        auto entry = m_watchEntryIds[index.row()];
        switch (Columns(index.column())) {
            case Color: return m_workspace->setWatchEntryGraphProperty(entry, Color, value);
            case DisplayName: return m_workspace->setWatchEntryGraphProperty(entry, DisplayName, value);
            case Expression: return m_workspace->setWatchEntryGraphProperty(entry, Expression, value);
            case PlotAreas: return m_workspace->setWatchEntryGraphProperty(entry, PlotAreas, value);
            case Thickness: return m_workspace->setWatchEntryGraphProperty(entry, Thickness, value);
            case LineStyle: return m_workspace->setWatchEntryGraphProperty(entry, LineStyle, value);
            case FrequencyLimit: return m_workspace->setWatchEntryGraphProperty(entry, FrequencyLimit, value);
            case MaxColumns:
            case FrequencyFeedback: Q_UNREACHABLE(); return m_workspace->setWatchEntryGraphProperty(-1, MaxColumns, 0);
        }
        return m_workspace->setWatchEntryGraphProperty(-1, MaxColumns, 0);
    }();

    if (setResult.isOk()) {
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

QVariant WatchEntryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    auto super = [&]() { return QAbstractTableModel::headerData(section, orientation, role); };
    if (orientation == Qt::Vertical) {
        return super();
    }

    // We only deal with horizontal header
    switch (Columns(section)) {
        case Color:
            switch (role) {
                case Qt::DisplayRole: return QString();
                default: break;
            }
        case DisplayName:
            switch (role) {
                case Qt::DisplayRole: return tr("Display name");
                default: break;
            }
        case Expression:
            switch (role) {
                case Qt::DisplayRole: return tr("Expression");
                default: break;
            }
        case PlotAreas:
            switch (role) {
                case Qt::DisplayRole: return tr("Plot areas");
                default: break;
            }
        case Thickness:
            switch (role) {
                case Qt::DisplayRole: return tr("Thickness");
                default: break;
            }
        case LineStyle:
            switch (role) {
                case Qt::DisplayRole: return tr("Style");
                default: break;
            }
        case FrequencyLimit:
            switch (role) {
                case Qt::DisplayRole: return tr("Frequency");
                default: break;
            }
        case MaxColumns:
        case FrequencyFeedback: return super();
    }
    return super(); // All unhandled cases goes to super
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

    emit beginRemoveRows(parent, row, row + count - 1);

    auto from = m_watchEntryIds.begin() + row;
    auto to = from + count;
    for (auto it = from; it != to; ++it) {
        if (auto result = m_workspace->removeWatchEntry(*it, true); result.isErr()) {
            qCritical() << "Remove watch entry" << *it << "fail:" << result.unwrapErr();
        }
    }
    m_watchEntryIds.erase(from, to);

    emit endRemoveRows();
    return true;
}

void WatchEntryModel::invalidateEntryDataDisplay(size_t entryId, Columns prop) {
    // TODO: use a bimap implementation? There won't be a heck lot of entries, a linear search won't hurt
    auto idx = m_watchEntryIds.indexOf(entryId);
    if (idx == -1) {
        qCritical() << "entryId" << entryId << "not inside watch entry model vec";
        return;
    }

    emit dataChanged(index(idx, prop), index(idx, prop), {Qt::DisplayRole});
}

void WatchEntryModel::notifyFrequencyFeedbackChanged(size_t entryId) {
    invalidateEntryDataDisplay(entryId, FrequencyLimit);
}

void WatchEntryModel::notifyFrequencyFeedbackChanged() {
    emit dataChanged(index(0, FrequencyLimit), index(m_watchEntryIds.size(), FrequencyLimit), {Qt::DisplayRole});
}

void WatchEntryModel::removeEntry(size_t entryId) {
    // TODO: use a bimap implementation? There won't be a heck lot of entries, a linear search won't hurt
    auto idx = m_watchEntryIds.indexOf(entryId);
    if (idx == -1) {
        qCritical() << "entryId" << entryId << "not inside watch entry model vec";
        return;
    }
}
