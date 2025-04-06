
#include <QColor>
#include <qjsonstream.h>

namespace Serialization {

struct WatchEntry {
    enum LineStyle {
        Solid = Qt::PenStyle::SolidLine,
        Dash = Qt::PenStyle::DashLine,
        Dot = Qt::PenStyle::DotLine,
        DashDot = Qt::PenStyle::DashDotLine,
        DashDotDot = Qt::PenStyle::DashDotDotLine,
    };
    QAS_JSON(LineStyle);

    QString expr;
    QString color; // #RRGGBB
    int thickness;
    LineStyle line_style;
    QList<int> plot_areas;
};
QAS_JSON_NS(WatchEntry);

struct PlotArea {
    int id;
    QString name;
};
QAS_JSON_NS(PlotArea);

} // namespace Serialization
