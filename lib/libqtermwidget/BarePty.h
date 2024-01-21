
#pragma once

#include <QBuffer>

class BarePty : public QObject {
    Q_OBJECT
public:
    BarePty(QObject *parent = nullptr) {}
    ~BarePty() {}



public slots:
    void sendData(const char *buffer, int length) { // From UI
        emit sendDataToBackend(buffer, length);
    }

    void receiveDataFromBackend(const char *buffer, int length) { // From Backend
        emit receivedData(buffer, length);
    }

signals:
    void receivedData(const char *buffer, int length); // Into UI

    void sendDataToBackend(const char *buffer, int length); // Into Backend
};
