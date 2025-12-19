#ifndef BASEFRAME_H
#define BASEFRAME_H
#include "frametype.h"

class BaseFrame
{
public:
    virtual ~BaseFrame() = default;

    FrameType type() const { return m_type; }
    int seq() const { return m_seq; }
    bool crc() const { return m_crc; }
    double time() const {return m_time; }

protected:
    BaseFrame(double time,FrameType type, int seq, bool crc)
        : m_time(time),m_type(type), m_seq(seq), m_crc(crc) {}

private:
    FrameType m_type;
    int m_seq;
    bool m_crc;
    double m_time;
};

#endif // BASEFRAME_H
