
#pragma once

#include <QMap>
#include <QString>

class IUiStateBridge {
public:
    virtual ~IUiStateBridge() {}

    /// @brief Collects a {PlotAreaID -> PlotAreaName} dict, used when other places needs all names of plot areas
    virtual QMap<size_t, QString> collectPlotAreaNames() = 0;

    /// @brief Gets the plot area name of a specific plot area
    virtual QString getPlotAreaName(size_t areaId) = 0;
};
