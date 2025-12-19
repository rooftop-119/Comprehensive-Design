#ifndef ANSFRAME_H
#define ANSFRAME_H

#include "baseframe.h"
#include "errortype.h"

class AnsFrame : public BaseFrame
{
public:
    explicit AnsFrame(double time,int seq,ErrorType type,bool crc)
        : BaseFrame(time,FrameType::Ans, seq, crc),
        m_errorType(type)
    {}

    ErrorType errorType() const {return m_errorType; }

private:
    ErrorType m_errorType;
};

#endif // ANSFRAME_H
