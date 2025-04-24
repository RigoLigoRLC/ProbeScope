
#pragma once

#include "qcustomplot.h"

class AxisTickerTimeMs : public QCPAxisTickerTime {
protected:
    virtual double getTickStep(const QCPRange &range) override;
    virtual QString getTickLabel(double tick, const QLocale &locale, QChar formatChar, int precision) override;
};
