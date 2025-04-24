
#pragma once

#include <QWidget>

namespace Ui {
class PlotAreaAdjustPopup;
}

class PlotAreaAdjustPopup : public QWidget {
    Q_OBJECT

public:
    explicit PlotAreaAdjustPopup(QWidget *parent = nullptr);
    ~PlotAreaAdjustPopup();

    using RangePair = QPair<double, double>;

    void getHorizontalRange(bool &autoFit, double &rangeSeconds);
    void getVerticalRange(bool &logScale, bool &autoFit, double &min, double &max);


    virtual bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    virtual void focusOutEvent(QFocusEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;

private slots:
    void on_cmbTimeRange_currentIndexChanged(int idx);
    void on_dialHorizontalZoom_sliderMoved(int v);
    void on_btnHorizontalFitAll_clicked();

    void on_cmbVerticalZoomRange_currentIndexChanged(int idx);
    void on_dialVerticalZoom_sliderMoved(int v);
    void on_btnVerticalZoomRangePlusMinus_clicked();
    void on_btnVerticalFitAll_clicked();
    void on_chkVerticalZoomUsePreset_clicked();
    void on_cmbVerticalZoomPreset_currentIndexChanged(int idx);
    void on_btnLogScale_clicked();

private:
    void initComboboxInternalList();
    void loadDefaultViewParams();

    void syncHorizontalZoom();
    void syncVericalZoom();

signals:
    void horizontalZoomChanged();
    void verticalZoomChanged();
    void lostFocus();

private:
    Ui::PlotAreaAdjustPopup *ui;

    QList<std::tuple<double, QString>> m_horizRangeList, m_vertRangeList;
    QList<std::tuple<double, double, QString>> m_vertRangePresetList;
};

Q_DECLARE_METATYPE(PlotAreaAdjustPopup::RangePair);
