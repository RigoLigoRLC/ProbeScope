#include "plotareaadjustpopup.h"
#include "ui_plotareaadjustpopup.h"
#include <QWindow>

PlotAreaAdjustPopup::PlotAreaAdjustPopup(QWidget *parent) : QWidget(parent), ui(new Ui::PlotAreaAdjustPopup) {
    ui->setupUi(this);

    // Fixed size
    setFixedSize(geometry().size());

    setWindowFlag(Qt::WindowCloseButtonHint, false);
    setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    setWindowFlag(Qt::WindowMinimizeButtonHint, false);

    initComboboxInternalList();
    loadDefaultViewParams();
}

PlotAreaAdjustPopup::~PlotAreaAdjustPopup() {
    delete ui;
}

void PlotAreaAdjustPopup::getHorizontalRange(bool &autoFit, double &rangeSeconds) {
    //
    autoFit = ui->btnHorizontalFitAll->isChecked();
    rangeSeconds = std::get<0>(m_horizRangeList[ui->cmbTimeRange->currentIndex()]);
}

void PlotAreaAdjustPopup::getVerticalRange(bool &logScale, bool &autoFit, double &min, double &max) {
    //
    auto usePreset = ui->chkVerticalZoomUsePreset->isChecked();
    logScale = ui->btnLogScale->isChecked();
    autoFit = !usePreset && ui->btnVerticalFitAll->isChecked();

    QString _;
    if (usePreset) {
        std::tie(min, max, _) = m_vertRangePresetList[ui->cmbVerticalZoomPreset->currentIndex()];
    } else {
        max = std::get<0>(m_vertRangeList[ui->cmbVerticalZoomRange->currentIndex()]);
        min = ui->btnVerticalZoomRangePlusMinus->isChecked() ? -max : 0;
    }
}

bool PlotAreaAdjustPopup::eventFilter(QObject *obj, QEvent *event) {
    Q_UNUSED(obj)
    if (event->type() == QEvent::FocusOut)
        emit lostFocus();

    return false;
}

void PlotAreaAdjustPopup::focusOutEvent(QFocusEvent *event) {
    QWidget::focusOutEvent(event);
}

void PlotAreaAdjustPopup::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    Q_ASSERT(windowHandle());
    windowHandle()->installEventFilter(this);
}

void PlotAreaAdjustPopup::initComboboxInternalList() {
    // TODO: configurable?
    m_horizRangeList = {
        {1,       "1s"   },
        {5,       "5s"   },
        {10,      "10s"  },
        {30,      "30s"  },
        {60,      "1min" },
        {60 * 2,  "2min" },
        {60 * 5,  "5min" },
        {60 * 10, "10min"},
        {60 * 20, "20min"},
        {60 * 30, "30min"},
    };

    m_vertRangeList = {
        {1,         "1"          },
        {5,         "5"          },
        {10,        "10"         },
        {100,       "100"        },
        {1000,      "1,000"      },
        {10000,     "10,000"     },
        {100000,    "100,000"    },
        {1000000,   "1,000,000"  },
        {10000000,  "10,000,000" },
        {010000000, "100,000,000"},
    };

    m_vertRangePresetList = {
        {-1,        1,        tr("Normalized (-1~1)")     },
        {-128,      127,      tr("int8_t (-128~127)")     },
        {0,         255,      tr("uint8_t (0~255)")       },
        {-32768,    32767,    tr("int16_t (-32768~32767)")},
        {0,         65535,    tr("int16_t (0~65535)")     },
        {0,         360,      tr("Circle (0~360)")        },
        {0,         720,      tr("Circle (0~720)")        },
        {180,       -180,     tr("Circle (-180~180)")     },
        {360,       -360,     tr("Circle (-360~360)")     },
        {0,         M_PI,     tr("Circle (0~π)")         },
        {0,         2 * M_PI, tr("Circle (0~2π)")        },
        {-M_PI / 2, M_PI / 2, tr("Circle (-π/2~π/2)")   },
        {-M_PI,     M_PI,     tr("Circle (-π~π)")       },
    };

    ui->cmbTimeRange->clear();
    for (auto &[r, s] : m_horizRangeList) {
        ui->cmbTimeRange->addItem(s, r);
    }

    ui->cmbVerticalZoomRange->clear();
    for (auto &[r, s] : m_vertRangeList) {
        ui->cmbVerticalZoomRange->addItem(s, r);
    }

    ui->cmbVerticalZoomPreset->clear();
    for (auto &[l, u, s] : m_vertRangePresetList) {
        ui->cmbVerticalZoomPreset->addItem(s, QVariant::fromValue(QPair<double, double>{l, u}));
    }

    // Dials' maximums follows the combobox
    ui->dialHorizontalZoom->setMaximum(ui->cmbTimeRange->count() - 1);
    ui->dialVerticalZoom->setMaximum(ui->cmbVerticalZoomRange->count() - 1);
}

void PlotAreaAdjustPopup::loadDefaultViewParams() {
    // TODO: make configurable
    // Horizontal: 10s
    ui->cmbTimeRange->setCurrentIndex(2);
    // Vertical: +-10, with auto fit
    ui->btnVerticalZoomRangePlusMinus->setChecked(true);
    ui->cmbVerticalZoomRange->setCurrentIndex(2);
    ui->btnVerticalFitAll->setChecked(true);
}

void PlotAreaAdjustPopup::syncHorizontalZoom() {
    ui->dialHorizontalZoom->setValue(ui->cmbTimeRange->currentIndex());

    emit horizontalZoomChanged();
}

void PlotAreaAdjustPopup::syncVericalZoom() {
    ui->dialVerticalZoom->setValue(ui->cmbVerticalZoomRange->currentIndex());
    ui->frmVerticalRangeFineAdjust->setDisabled(ui->chkVerticalZoomUsePreset->isChecked());

    emit verticalZoomChanged();
}

void PlotAreaAdjustPopup::on_cmbTimeRange_currentIndexChanged(int i) {
    Q_UNUSED(i);

    syncHorizontalZoom();
}

void PlotAreaAdjustPopup::on_dialHorizontalZoom_sliderMoved(int v) {
    ui->cmbTimeRange->setCurrentIndex(v);
    syncHorizontalZoom();
}

void PlotAreaAdjustPopup::on_btnHorizontalFitAll_clicked() {
    syncHorizontalZoom();
}

void PlotAreaAdjustPopup::on_cmbVerticalZoomRange_currentIndexChanged(int i) {
    Q_UNUSED(i);

    syncVericalZoom();
}

void PlotAreaAdjustPopup::on_dialVerticalZoom_sliderMoved(int v) {
    ui->cmbVerticalZoomRange->setCurrentIndex(v);
    syncVericalZoom();
}

void PlotAreaAdjustPopup::on_btnVerticalZoomRangePlusMinus_clicked() {
    syncVericalZoom();
}

void PlotAreaAdjustPopup::on_btnVerticalFitAll_clicked() {
    syncVericalZoom();
}

void PlotAreaAdjustPopup::on_chkVerticalZoomUsePreset_clicked() {
    syncVericalZoom();
}

void PlotAreaAdjustPopup::on_cmbVerticalZoomPreset_currentIndexChanged(int i) {
    syncVericalZoom();
}

void PlotAreaAdjustPopup::on_btnLogScale_clicked() {
    syncVericalZoom();
}
