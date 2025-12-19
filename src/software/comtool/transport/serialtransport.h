#ifndef SERIALTRANSPORT_H
#define SERIALTRANSPORT_H

#include "itransport.h"
#include <QSerialPort>
#include <QTimer>
#include "util/frameparser.h"

struct SerialConfig {
    QString portName;
    int baudRate = 115200;
    int dataBits = 8;
    QSerialPort::Parity parity = QSerialPort::NoParity;
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
};

class SerialTransport : public ITransport {
    Q_OBJECT
public:
    explicit SerialTransport(QObject* parent = nullptr);
    ~SerialTransport() override;

    // 配置接口
    void setConfig(const SerialConfig& config);
    SerialConfig config() const { return m_config; }

    // ITransport 接口实现
    bool open() override;
    void close() override;
    bool isOpen() const override;
    State state() const override { return m_state; }

    bool write(const QByteArray& data) override;

    qint64 bytesReceived() const override { return m_bytesReceived; }
    qint64 bytesSent() const override { return m_bytesSent; }

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    void processBuffer();
    void setState(State newState);

    QSerialPort* m_serial = nullptr;
    SerialConfig m_config;
    FrameParser m_parser;

    QByteArray m_receiveBuffer;
    State m_state = State::Disconnected;

    qint64 m_bytesReceived = 0;
    qint64 m_bytesSent = 0;

    static const int MAX_BUFFER_SIZE = 2048;
    static const QByteArray FRAME_HEADER;
};

#endif
