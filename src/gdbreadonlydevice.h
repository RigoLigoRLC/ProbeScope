
#pragma once

#include <QBuffer>
#include <QIODevice>


class GdbReadonlyDevice final : public QIODevice {
    Q_OBJECT

public:
    explicit GdbReadonlyDevice(QObject *parent = nullptr) : QIODevice(parent), m_open(false) {
        m_gdbStdoutBuffer.open(QBuffer::ReadWrite);
    };
    ~GdbReadonlyDevice() { m_gdbStdoutBuffer.close(); };

    friend class GdbContainer;

public:
    // QIODevice interface
    virtual bool isSequential() const override { return true; }
    virtual bool open(OpenMode mode) override {
        QIODevice::open(mode);
        m_open = true;
        return true;
    }
    virtual void close() override {
        m_open = false;
        m_gdbStdoutBuffer.close();
    }
    virtual qint64 pos() const override { return 0; }
    virtual qint64 size() const override { return 0; }
    virtual bool seek(qint64 pos) override { return false; }
    virtual bool atEnd() const override { return false; }
    virtual bool reset() override { return m_gdbStdoutBuffer.reset(); }
    virtual qint64 bytesAvailable() const override { return m_gdbStdoutBuffer.bytesAvailable(); }
    virtual qint64 bytesToWrite() const override { return 0; }
    virtual bool canReadLine() const override { return false; }
    virtual bool waitForBytesWritten(int msecs) override { return false; }
    virtual bool waitForReadyRead(int msecs) override { return m_gdbStdoutBuffer.waitForReadyRead(msecs); }

protected:
    virtual qint64 readData(char *data, qint64 maxlen) override { return m_gdbStdoutBuffer.read(data, maxlen); }
    virtual qint64 writeData(const char *data, qint64 len) override { return len; } // Pretend to have written

    void internalAddData(QByteArray data) {
        m_gdbStdoutBuffer.seek(m_gdbStdoutBuffer.size());
        m_gdbStdoutBuffer.write(data);
        m_gdbStdoutBuffer.seek(0);
        emit readyRead();
    }

private:
    QBuffer m_gdbStdoutBuffer;
    bool m_open;
};
