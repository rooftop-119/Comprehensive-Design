#include "serialworker.h"
#include <QDateTime>
#include <QThread>

SerialWorker::SerialWorker(QObject *parent) : QObject(parent) { }

SerialWorker::~SerialWorker() { }

bool SerialWorker::openPort(const QString &portName, int baudRate, int dataBits, const QString &parity, double stopBits)
{
    if (com) {
        com->deleteLater();
        com = nullptr;
    }

    com = new QSerialPort();
    com->moveToThread(QThread::currentThread());
    com->setPortName(portName);

    if (!com->open(QIODevice::ReadWrite)) {
        emit log(Logger::Type::SerialRecv,com->errorString());
        delete com;
        com = nullptr;
        return false;
    }

    com->setBaudRate(baudRate);

    switch (dataBits) {
    case 5: com->setDataBits(QSerialPort::Data5); break;
    case 6: com->setDataBits(QSerialPort::Data6); break;
    case 7: com->setDataBits(QSerialPort::Data7); break;
    default: com->setDataBits(QSerialPort::Data8); break;
    }

    if (parity == "无")
        com->setParity(QSerialPort::NoParity);
    else if (parity == "奇")
        com->setParity(QSerialPort::OddParity);
    else if (parity == "偶")
        com->setParity(QSerialPort::EvenParity);

    if (stopBits == 1)
        com->setStopBits(QSerialPort::OneStop);
    else if (stopBits == 1.5)
        com->setStopBits(QSerialPort::OneAndHalfStop);
    else
        com->setStopBits(QSerialPort::TwoStop);

    com->setFlowControl(QSerialPort::NoFlowControl);

    connect(com, &QSerialPort::readyRead,
            this, &SerialWorker::handleReadyRead,
            Qt::DirectConnection);

    emit portOpened();
    return true;
}

void SerialWorker::closePort()
{
    if (!com) return;

    disconnect(com, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);

    if (com->isOpen())
        com->close();

    com->deleteLater();
    com = nullptr;

    emit portClosed();
}

void SerialWorker::writeData(const QByteArray &data)
{
    if (!com || !com->isOpen()) {
        emit log(Logger::Type::SerialSend,"串口未打开，发送失败");
        return;
    }

    qint64 bytesWritten = com->write(data);
    emit log(Logger::Type::SerialSend,data);

    if (bytesWritten == -1) {
        emit log(Logger::Type::SerialSend,"写入串口失败: " + com->errorString());
    } else if (!com->waitForBytesWritten(50)) {
        emit log(Logger::Type::SerialSend,"串口发送超时");
    }
}

void SerialWorker::handleReadyRead()
{
    if (!com) return;

    // 1. 读入所有可用字节
    m_buffer.append(com->readAll());

    const QByteArray frameHeader = QByteArray::fromHex("AA55");
    const quint8 frameTail = 0x0D;

    while (true) {
        // --- 2. 找帧头 ---
        int headIndex = m_buffer.indexOf(frameHeader);
        if (headIndex < 0) {
            // 没找到帧头，防止无限膨胀
            if (m_buffer.size() > 2048) m_buffer.clear();
            return;
        }

        // 删除帧头前的垃圾数据
        if (headIndex > 0)
            m_buffer.remove(0, headIndex);

        // --- 3. 判断长度是否足够形成一帧 ---
        if (m_buffer.size() < frame_len)
            return; // 继续等下一次 readyRead

        // --- 4. 检查帧尾 ---
        if (static_cast<quint8>(m_buffer[frame_len - 1]) != frameTail) {
            // 帧尾不对，丢弃第一个字节继续找帧头
            m_buffer.remove(0, 1);
            continue;
        }

        // --- 5. 提取一帧 ---
        QByteArray frame = m_buffer.left(frame_len);
        m_buffer.remove(0, frame_len);

        // --- 6. 校验 ---
        quint8 checksum = 0;
        for (int i = 2; i < 7; ++i)
            checksum += static_cast<quint8>(frame[i]);
        checksum &= 0xFF;

        quint8 recvChecksum = static_cast<quint8>(frame[7]);
        if (checksum != recvChecksum) {
            //emit log(Logger::Type::SerialRecv,
            //         QString("校验错误 index=%1").arg(static_cast<quint8>(frame[3])));
            //continue;
        }

        // --- 7. 解析数据部分 ---
        QByteArray data = frame.mid(2, 5);

        // --- 8. 发给上层 ---
        emit gotData(data);
    }
}
