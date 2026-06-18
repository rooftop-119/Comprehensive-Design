#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include "logger.h"

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();
    bool isOpen() const { return com && com->isOpen(); }

public slots:
    bool openPort(const QString &portName, int baudRate, int dataBits, const QString &parity, double stopBits);
    void closePort();
    void writeData(const QByteArray &data);

signals:
    void gotData(const QByteArray &data);
    void portOpened();
    void portClosed();
    void log(Logger::Type type, const QString &msg, bool clear=false);

private slots:
    void handleReadyRead();

private:
    QSerialPort *com = nullptr;
    QByteArray m_buffer;

    static const int frame_len = 9;
};

#endif // SERIALWORKER_H
