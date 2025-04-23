
#pragma once

#include <QPainter>
#include <QWidget>

class WelcomeBackground : public QWidget {
    Q_OBJECT
public:
    WelcomeBackground(QWidget *parent = nullptr) : QWidget(parent){};
    ~WelcomeBackground(){};

protected:
    void paintEvent(QPaintEvent *event) override;
};
