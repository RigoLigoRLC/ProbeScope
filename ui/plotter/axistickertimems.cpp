
#include "axistickertimems.h"

double AxisTickerTimeMs::getTickStep(const QCPRange &range) {
    QCPRange newRange(range.lower / 1000.0, range.upper / 1000.0);
    return QCPAxisTickerTime::getTickStep(newRange) * 1000.0;
}

QString AxisTickerTimeMs::getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) {
    return QCPAxisTickerTime::getTickLabel(tick / 1000, locale, formatChar, precision);
}
