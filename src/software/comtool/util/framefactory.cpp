#include "framefactory.h"
#include <QDebug>

quint8 FrameFactory::getNextSeq() {
    QMutexLocker locker(&m_seqMutex);
    m_lastSeq = m_seqCounter;
    return m_seqCounter++;
}

quint8 FrameFactory::calculateCRC8(const QByteArray& data) {
    quint8 crc = 0x00;
    const quint8 polynomial = 0x07;

    for (char ch : data) {
        quint8 byte = static_cast<quint8>(ch);
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

QByteArray FrameFactory::buildCommandFrame(FrameType type, const QByteArray& data) {
    // 计算总长度：长度字段(1) + 命令类型(1) + 序列号(1) + 附加数据 + CRC(1) + 帧尾(1)
    int totalDataLength = 2 + 1 + 1 + 1 + data.size() + 1 + 1;

    // 构建帧
    QByteArray frame;
    frame.reserve(totalDataLength);  // 帧头(2) + 数据部分

    // 1. 帧头
    frame.append(Protocol::FRAME_HEADER);

    // 2. 长度字段（数据部分总长度，包括长度字段自身）
    frame.append(static_cast<char>(totalDataLength));

    // 3. 命令类型
    frame.append(static_cast<char>(type));

    // 4. 序列号
    quint8 seq = getNextSeq();
    frame.append(static_cast<char>(seq));

    // 5. 附加数据（如果有）
    if (!data.isEmpty()) {
        frame.append(data);
    }

    // 6. 计算CRC（从长度字段开始到附加数据结束）
    QByteArray crcData = frame.mid(2);  // 跳过帧头
    quint8 crc = calculateCRC8(crcData);
    frame.append(static_cast<char>(crc));

    // 7. 帧尾
    frame.append(static_cast<char>(Protocol::FRAME_TAIL));

    qDebug() << "FrameFactory::buildCommandFrame -"
             << "Type:" << static_cast<int>(type)
             << "Seq:" << seq
             << "Data:" << QString(frame.toHex(':'));

    return frame;
}

bool FrameFactory::validate(const QByteArray& raw) {
    // 基本长度检查
    if (raw.size() < Protocol::MIN_FRAME_SIZE) {
        qWarning() << "FrameFactory::validate - 帧长度不足:" << raw.size();
        return false;
    }

    // 检查帧头
    if (!raw.startsWith(Protocol::FRAME_HEADER)) {
        qWarning() << "FrameFactory::validate - 无效帧头";
        return false;
    }

    // 检查帧尾
    if (static_cast<quint8>(raw[raw.size() - 1]) != Protocol::FRAME_TAIL) {
        qWarning() << "FrameFactory::validate - 无效帧尾";
        return false;
    }

    // 检查长度字段一致性
    quint8 dataLength = static_cast<quint8>(raw[2]);
    int expectedSize = 2 + dataLength;  // 帧头(2) + 数据部分(包括长度字段)

    if (raw.size() != expectedSize) {
        qWarning() << QString("FrameFactory::validate - 长度不匹配: 期望 %1, 实际 %2")
                          .arg(expectedSize).arg(raw.size());
        return false;
    }

    // CRC校验
    // CRC计算范围：从长度字段到CRC前的最后一个字节
    int crcPos = 2 + dataLength - 2;  // CRC位置
    QByteArray crcData = raw.mid(2, dataLength - 2);  // 长度字段 + 数据部分（不含CRC和帧尾）

    quint8 calculatedCRC = calculateCRC8(crcData);
    quint8 receivedCRC = static_cast<quint8>(raw[crcPos]);

    if (calculatedCRC != receivedCRC) {
        qWarning() << QString("FrameFactory::validate - CRC校验失败: 计算=0x%1 接收=0x%2")
                          .arg(calculatedCRC, 2, 16, QLatin1Char('0'))
                          .arg(receivedCRC, 2, 16, QLatin1Char('0'));
        return false;
    }

    return true;
}
