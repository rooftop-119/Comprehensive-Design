#include "framedispatcher.h"

FrameDispatcher::FrameDispatcher(QObject* parent)
    : QObject(parent)
{
}

void FrameDispatcher::dispatch(std::shared_ptr<BaseFrame> frame)
{
    if (!frame || frame->crc()) return;

    bool isData = true;

    Sample s;
    s.timestamp = frame->time();
    s.seq = frame->seq();

    switch (frame->type()) {

    case FrameType::DataVoltage: {
        const auto& f = static_cast<const DataVoltageFrame&>(*frame);
        s.voltage = f.voltage();
        s.hasVoltage = true;
        break;
    }

    case FrameType::DataTemp: {
        const auto& f = static_cast<const DataTempFrame&>(*frame);
        s.temperature = f.temperature();
        s.hasTemperature = true;
        break;
    }

    case FrameType::DataBoth: {
        const auto& f = static_cast<const DataBothFrame&>(*frame);
        s.voltage = f.voltage();
        s.temperature = f.temperature();
        s.hasVoltage = true;
        s.hasTemperature = true;
        break;
    }

    case FrameType::Ans: {
        isData = false;
        const auto& f = static_cast<const AnsFrame&>(*frame);
        emit gotAns(f);

        qDebug()<<"dispatcher:"<<static_cast<quint8>(f.type())<<" "<<static_cast<quint8>(f.errorType());
        break;
    }

    default:
        return; // 非数据帧直接丢弃
    }

    if(isData){
        emit sampleReady(s);
    }
}
