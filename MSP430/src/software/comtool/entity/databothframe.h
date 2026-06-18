#ifndef DATABOTHFRAME_H
#define DATABOTHFRAME_H
#include "baseframe.h"

class DataBothFrame : public BaseFrame
{
public:
    explicit DataBothFrame(double time,int seq,double voltage,double temp,bool crc)
        : BaseFrame(time,FrameType::DataBoth, seq, crc),
        m_temp(temp),m_volt(voltage)
    {}

    double temperature() const { return m_temp; }
    double voltage() const { return m_volt; }

private:
    double m_temp;
    double m_volt;
};
#endif // DATABOTHFRAME_H
