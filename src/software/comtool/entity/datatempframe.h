#ifndef DATATEMPFRAME_H
#define DATATEMPFRAME_H
#include "baseframe.h"

class DataTempFrame : public BaseFrame
{
public:
    DataTempFrame(double time,int seq, double temp, bool crc)
        : BaseFrame(time,FrameType::DataTemp, seq, crc),
        m_temp(temp)
    {}

    double temperature() const { return m_temp; }

private:
    double m_temp;
};
#endif // DATATEMPFRAME_H
