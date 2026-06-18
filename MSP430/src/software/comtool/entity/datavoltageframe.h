#ifndef DATAVOLTAGEFRAME_H
#define DATAVOLTAGEFRAME_H
#include "baseframe.h"

class DataVoltageFrame : public BaseFrame
{
public:
    DataVoltageFrame(double time,int seq,double voltage,bool crc)
        : BaseFrame(time,FrameType::DataVoltage, seq, crc),
        m_volt(voltage)
    {}

    double voltage() const { return m_volt; }

private:
    double m_volt;
};

#endif // DATAVOLTAGEFRAME_H
