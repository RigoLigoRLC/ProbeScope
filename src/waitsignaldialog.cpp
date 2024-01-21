
#include "waitsignaldialog.h"

WaitSignalDialog::WaitSignalDialog(QWidget *parent) : QProgressDialog(parent) {
    m_waitTimer.setSingleShot(true);

    connect(&m_waitTimer, &QTimer::timeout, &m_eventLoop, &QEventLoop::quit);
}

WaitSignalDialog::~WaitSignalDialog() {}

void WaitSignalDialog::setInformation(QString waitInformation) {
    setLabelText(waitInformation);
}

bool WaitSignalDialog::wait(int timeout) {
    m_waitTimer.start(timeout);
    m_eventLoop.exec();

    // When timer is active upon event loop exit, it means the signal waiting for is emitted
    return m_waitTimer.isActive();
}
