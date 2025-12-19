#ifndef FRAMETYPE_H
#define FRAMETYPE_H

#include <QtGlobal>
#include <QByteArray>

enum class FrameType : quint8
{
    Invalid = 0x00,

    // 下位机 → 上位机
    DataVoltage = 0x01,
    DataTemp    = 0x02,
    DataBoth    = 0x03,
    DataEvent   = 0x04,

    Ans         = 0x0F,
    AnsData     = 0x0E,

    // 上位机 → 下位机（命令）
    CmdStop          = 0x10,
    CmdStartVoltage  = 0x11,
    CmdStartTemp     = 0x12,
    CmdStartBoth     = 0x13,
    CmdGetStatus     = 0x14,
    CmdCalibrate     = 0x15,
    CmdReset         = 0x18,
    CmdSaveConfig    = 0x1C,
};

inline bool isValidFrameType(quint8 value) {
    switch (static_cast<FrameType>(value)) {
    case FrameType::Invalid:
    case FrameType::DataVoltage:
    case FrameType::DataTemp:
    case FrameType::DataBoth:
    case FrameType::DataEvent:
    case FrameType::Ans:
    case FrameType::AnsData:
    case FrameType::CmdStop:
    case FrameType::CmdStartBoth:
    case FrameType::CmdStartTemp:
    case FrameType::CmdStartVoltage:
    case FrameType::CmdGetStatus:
    case FrameType::CmdCalibrate:
    case FrameType::CmdReset:
    case FrameType::CmdSaveConfig:
        return true;
    default:
        return false;
    }
}


// 安全转换函数
inline FrameType toFrameType(quint8 value) {
    return isValidFrameType(value) ? static_cast<FrameType>(value) : FrameType::Invalid;
}

inline quint8 getLen(FrameType type) {
    switch (type) {
    case FrameType::DataVoltage:    return 7;
    case FrameType::DataTemp:       return 8;
    case FrameType::DataBoth:       return 9;
    case FrameType::DataEvent:      return 7;
    case FrameType::Ans:            return 7;
    case FrameType::AnsData:        return 10;
    case FrameType::CmdStop:        return 7;
    case FrameType::CmdStartVoltage: return 8;
    case FrameType::CmdStartTemp:    return 8;
    case FrameType::CmdStartBoth:    return 9;
    case FrameType::CmdGetStatus:    return 7;
    case FrameType::CmdCalibrate:    return 10;
    case FrameType::CmdReset:        return 8;
    case FrameType::CmdSaveConfig:   return 7;
    case FrameType::Invalid:
    default: return 0;
    }
}
#endif // FRAMETYPE_H
