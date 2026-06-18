#include "serialtransport.h"
#include <QThread>

const QByteArray SerialTransport::FRAME_HEADER = QByteArray::fromHex("AA55");

SerialTransport::SerialTransport(QObject* parent)
    : ITransport(parent)
{
}

SerialTransport::~SerialTransport() {
    close();
}

void SerialTransport::setConfig(const SerialConfig& config) {
    if (isOpen()) {
        emit log(Logger::Type::System, "警告：串口已打开，配置将在下次打开时生效");
    }
    m_config = config;
}

bool SerialTransport::open() {
    if (isOpen()) {
        emit log(Logger::Type::System, "串口已经打开");
        return true;
    }

    setState(State::Connecting);

    m_serial = new QSerialPort(this);
    m_serial->setPortName(m_config.portName);
    m_serial->setBaudRate(m_config.baudRate);
    m_serial->setDataBits(static_cast<QSerialPort::DataBits>(m_config.dataBits));
    m_serial->setParity(m_config.parity);
    m_serial->setStopBits(m_config.stopBits);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        QString error = m_serial->errorString();
        emit log(Logger::Type::System, QString("串口打开失败: %1").arg(error));
        emit errorOccurred(error);

        m_serial->deleteLater();
        m_serial = nullptr;
        setState(State::Error);
        return false;
    }

    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialTransport::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred,
            this, &SerialTransport::onErrorOccurred);

    // 清空缓冲区
    m_receiveBuffer.clear();
    m_bytesReceived = 0;
    m_bytesSent = 0;

    setState(State::Connected);
    emit log(Logger::Type::System,
             QString("串口已打开: %1 @ %2").arg(m_config.portName).arg(m_config.baudRate));

    return true;
}

void SerialTransport::close() {
    if (!m_serial) return;

    disconnect(m_serial, nullptr, this, nullptr);

    if (m_serial->isOpen()) {
        m_serial->close();
    }

    m_serial->deleteLater();
    m_serial = nullptr;

    m_receiveBuffer.clear();
    setState(State::Disconnected);

    emit log(Logger::Type::System, "串口已关闭");
}

bool SerialTransport::isOpen() const {
    return m_serial && m_serial->isOpen();
}

bool SerialTransport::write(const QByteArray& data) {
    if (!isOpen()) {
        emit log(Logger::Type::System, "串口未打开，发送失败");
        return false;
    }

    qint64 written = m_serial->write(data);

    if (written == -1) {
        emit log(Logger::Type::System, "串口写入失败: " + m_serial->errorString());
        return false;
    }

    if (!m_serial->waitForBytesWritten(100)) {
        emit log(Logger::Type::System, "串口发送超时");
        return false;
    }

    m_bytesSent += written;
    emit log(Logger::Type::SerialSend, QString(data.toHex(':')));

    return true;
}

void SerialTransport::onReadyRead() {
    if (!m_serial) return;

    QByteArray newData = m_serial->readAll();
    m_bytesReceived += newData.size();

    m_receiveBuffer.append(newData);

    // 防止缓冲区过大
    if (m_receiveBuffer.size() > MAX_BUFFER_SIZE) {
        emit log(Logger::Type::System, "接收缓冲区溢出，已清空");
        m_receiveBuffer.clear();
        return;
    }

    processBuffer();
}

void SerialTransport::processBuffer() {
    while (true) {
        // 查找帧头
        int headerPos = m_receiveBuffer.indexOf(FRAME_HEADER);

        if (headerPos == -1) {
            // 保留最后一个字节（可能是半个帧头）
            if (m_receiveBuffer.size() > 1) {
                m_receiveBuffer = m_receiveBuffer.right(1);
            }
            return;
        }

        // 丢弃帧头前的无效数据
        if (headerPos > 0) {
            m_receiveBuffer.remove(0, headerPos);
        }

        // 检查是否有足够数据读取长度字段
        if (m_receiveBuffer.size() < 3) return;

        quint8 frameLength = static_cast<quint8>(m_receiveBuffer[2]);
        int totalLength = frameLength;  // 帧头 + 数据部分

        // 验证长度合理性
        if (frameLength < 6 || frameLength > 20) {
            m_receiveBuffer.remove(0, 1);
            continue;
        }

        // 等待完整帧
        if (m_receiveBuffer.size() < totalLength) return;

        // 提取完整帧
        QByteArray frame = m_receiveBuffer.left(totalLength);
        m_receiveBuffer.remove(0, totalLength);

        // 解析帧
        auto parsedFrame = m_parser.parse(frame);
        if (parsedFrame) {
            emit frameReceived(parsedFrame);
            if(DataShow){
                emit log(Logger::Type::SerialRecv, QString(frame.toHex(':')));
            }else{
                if(frame.size() == 7){
                    emit log(Logger::Type::SerialRecv, QString(frame.toHex(':')));
                }
            }
        }
    }
}

void SerialTransport::onErrorOccurred(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError) return;

    QString errorStr = m_serial ? m_serial->errorString() : "Unknown error";
    emit log(Logger::Type::System, QString("串口错误: %1").arg(errorStr));
    emit errorOccurred(errorStr);

    setState(State::Error);
}

void SerialTransport::setState(State newState) {
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(m_state);
    }
}

void SerialTransport::setDataShow(bool show){
    this->DataShow=show;
}

