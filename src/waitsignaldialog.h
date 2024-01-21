
#pragma once

#include <QEventLoop>
#include <QProgressDialog>
#include <QTimer>


class WaitSignalDialog final : public QProgressDialog {
    Q_OBJECT

public:
    WaitSignalDialog(QWidget *parent = nullptr);
    ~WaitSignalDialog();

    void setInformation(QString waitInformation);
    bool wait(int timeout = 0);

private:
    QEventLoop m_eventLoop;
    QTimer m_waitTimer;
};
