
#pragma once

#include <QPainter>
#include <QWidget>


class WelcomeBackground : public QWidget {
    Q_OBJECT
public:
    WelcomeBackground(QWidget *parent = nullptr) : QWidget(parent){};
    ~WelcomeBackground(){};

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        // Background color
        // painter.setPen(Qt::NoPen);
        // painter.setBrush(QColor(QPalette::Base));
        // painter.drawRect(rect());

        // Status bar tips
        painter.setPen(QColor(QPalette::Text));
        painter.drawText(rect().adjusted(10, 0, 0, 0), Qt::AlignLeft | Qt::AlignBottom,
                         tr("Select your debug probe, target device on the status bar.\n"
                            "Press the button with a green indicator to connect."));

        // Drag symbol file tips
        auto symbolTipRect = rect();
        symbolTipRect.setTop(symbolTipRect.height() / 2 + 50);
        painter.drawText(symbolTipRect, Qt::AlignHCenter | Qt::AlignTop,
                         tr("Drag and drop a symbol file here to start."));

        // Welcome
        auto welcomeRect = rect();
        welcomeRect.setBottom(welcomeRect.height() / 2 - 50);
        auto welcomeFont = painter.font();
        welcomeFont.setPointSize(20);
        painter.setFont(welcomeFont);
        painter.drawText(rect().adjusted(10, 10, -10, -10), Qt::AlignCenter, tr("Welcome to ProbeScope"));
    }
};
