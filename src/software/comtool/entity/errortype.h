#ifndef ERRORTYPE_H
#define ERRORTYPE_H

#include <QtGlobal>
#include <QByteArray>

enum class ErrorType : quint8
{
    Invalid         = 0xFF,

    SUCCESS         = 0x00,
    INVALID_CMD     = 0x01,
    PARAM_ERROR     = 0x02,
    DEVICE_BUSY     = 0x03,
    HARDWARE_ERROR  = 0x04,
    CRC_ERROR       = 0x05,
    TIMEOUT         = 0x06,
    NOT_CALIBRATED  = 0x07,
    FLASH_ERROR     = 0x08,
    INVALID_STATE   = 0x09,
    OVERFLOW_ANS    = 0x0A,
};

inline bool isValidErrorType(quint8 value) {
    switch (static_cast<ErrorType>(value)) {
    case ErrorType::Invalid:
    case ErrorType::SUCCESS:
    case ErrorType::INVALID_CMD:
    case ErrorType::PARAM_ERROR:
    case ErrorType::DEVICE_BUSY:
    case ErrorType::HARDWARE_ERROR:
    case ErrorType::CRC_ERROR:
    case ErrorType::TIMEOUT:
    case ErrorType::NOT_CALIBRATED:
    case ErrorType::FLASH_ERROR:
    case ErrorType::INVALID_STATE:
    case ErrorType::OVERFLOW_ANS:
        return true;
    default:
        return false;
    }
}


// 安全转换函数
inline ErrorType toErrorType(quint8 value) {
    return isValidErrorType(value) ? static_cast<ErrorType>(value) : ErrorType::Invalid;
}

#endif // ERRORTYPE_H
