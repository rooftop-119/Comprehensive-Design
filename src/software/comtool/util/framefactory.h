#ifndef FRAMEFACTORY_H
#define FRAMEFACTORY_H

#include "entity/frametype.h"
#include "entity/ansframe.h"
#include <QByteArray>
#include <QMutex>

namespace Protocol {
const inline QByteArray FRAME_HEADER = QByteArray::fromHex("AA55");
const inline quint8 FRAME_TAIL = 0x0D;
const inline quint8 MIN_FRAME_SIZE = 6;
}

class FrameFactory {
public:
    FrameFactory() = default;

    // 构建命令帧
    QByteArray buildCommandFrame(FrameType type,
                                 const QByteArray& data = QByteArray());

    // 校验帧有效性
    bool validate(const QByteArray& raw);

    // 获取最后使用的序列号
    quint8 getLastSeq() const { return m_lastSeq; }

private:
    quint8 calculateCRC8(const QByteArray& data);
    quint8 getNextSeq();

    QMutex m_seqMutex;
    quint8 m_seqCounter = 0x00;
    quint8 m_lastSeq = 0x00;
};

#endif
