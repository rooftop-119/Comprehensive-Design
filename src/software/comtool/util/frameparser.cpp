#include "frameparser.h"
#include <QDateTime>
#include <QDataStream>

std::shared_ptr<BaseFrame> FrameParser::parse(const QByteArray& raw)
{
    if (raw.size() < 7) return nullptr;

    quint8 typeByte = static_cast<quint8>(raw[3]);
    FrameType type = toFrameType(typeByte);
    if (type == FrameType::Invalid) return nullptr;

    double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;

    int len = static_cast<quint8>(raw[2]);
    int seq = static_cast<quint8>(raw[4]);
    bool crcOk = checkCRC(raw,len);

    switch (type) {
    case FrameType::DataVoltage: {
        quint16 voltage = qFromLittleEndian<quint16>(raw.constData() + start + 3);
        double volt = voltage / 1000.0;

        return std::make_shared<DataVoltageFrame>(now, seq, volt, crcOk);
    }
    case FrameType::DataTemp: {
        qint16 temperature = qFromLittleEndian<qint16>(raw.constData() + start + 5);
        double temp = temperature / 100.0;

        return std::make_shared<DataTempFrame>(now, seq, temp, crcOk);
    }
    case FrameType::DataBoth: {

        // 电压：uint16_t 小端
        quint16 voltage = qFromLittleEndian<quint16>(raw.constData() + start + 3);
        double volt = voltage / 1000.0;

        // 温度：int16_t 小端（有符号）
        qint16 temperature = qFromLittleEndian<qint16>(raw.constData() + start + 5);
        double temp = temperature / 100.0;

        return std::make_shared<DataBothFrame>(now, seq, volt, temp, crcOk);
    }
    case FrameType::Ans:{
        ErrorType et = toErrorType(static_cast<quint8>(raw[start + 3]));
        return std::make_shared<AnsFrame>(now, seq, et, crcOk);
    }
    default:
        return nullptr;
    }
}

bool FrameParser::checkCRC(const QByteArray& raw,int length)
{
    Q_UNUSED(raw);
    // TODO: CRC 校验
    length = length - start - 2;

    quint8 crc = 0x00;  // 初始值
    const quint8 polynomial = 0x07;  // CRC8-CCITT 多项式

    for (int i = start; i < start + length; ++i) {
        quint8 byte = static_cast<quint8>(raw[i]);

        // LSB First 处理
        crc ^= byte;

        for (int j = 0; j < 8; j++) {
            if (crc & 0x01) {  // 检查LSB
                crc = (crc >> 1) ^ polynomial;  // 右移，多项式反转处理
            } else {
                crc >>= 1;
            }
        }
    }

    return crc == raw[start + length];
}
