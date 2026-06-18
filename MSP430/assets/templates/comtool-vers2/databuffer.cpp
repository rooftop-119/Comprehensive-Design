#include "databuffer.h"
#include <QDir>

DataBuffer::DataBuffer(QObject *parent)
    : QObject(parent),
    capacity(2000),
    writePos(0),
    plotReadPos(0),
    fileWritePos(0),
    count(0)
{
    buffer.resize(capacity);
    emit log(Logger::Type::System,QStringLiteral("SerialBuffer initing，容量/帧：%1")
                                       .arg(capacity));
}

DataBuffer::~DataBuffer()
{    
    emit flushToFile(readForFile());
    emit log(Logger::Type::System,"SerialBuffer closing，正在落盘剩余数据");
}

void DataBuffer::append(double timeSec, const QByteArray &data){

    QMutexLocker locker(&mutex);

    buffer[writePos] = QPair<double,QByteArray>(timeSec,data);
    writePos = (writePos + 1) % capacity;

    if (count < capacity) {
        count++;
    } else {
        // 环形覆盖时，自动推进两个读指针以保持同步
        plotReadPos = (plotReadPos + 1) % capacity;
        fileWritePos = (fileWritePos + 1) % capacity;
        emit log(Logger::Type::System,"串口缓存溢出，请做参数检查，本次丢失数据/帧：1");
    }


}

QVector<QPair<double,QByteArray>> DataBuffer::readForFile(){

    QMutexLocker locker(&mutex);
    QVector<QPair<double,QByteArray>> result;

    int available = (writePos - fileWritePos + capacity) % capacity;

    int pos = fileWritePos;
    for (int i = 0; i < available; ++i) {
        result.append(buffer[pos]);
        pos = (pos + 1) % capacity;
    }

    // 文件写入完毕后，将 fileWritePos 更新为最新位置
    fileWritePos = (fileWritePos + available) % capacity;

    emit log(Logger::Type::System,QStringLiteral("串口缓存 %1/%2")
                                       .arg(count)
                                       .arg(capacity));

    // 删除已写入的数据（只调整计数，环形结构不会真的清空）
    count -= available;
    if (count < 0) count = 0;

    return result;
}

QVector<QPair<double,double>> DataBuffer::readForPlot(int num){



    QMutexLocker locker(&mutex);
    QVector<QPair<double,double>> result;
    static int times_noData = 0;
    int available = std::min((writePos - plotReadPos + capacity) % capacity,num);
    if(available == 0){
        times_noData++;
        if(times_noData == 10){
            emit noDataForPlot();
            emit log(Logger::Type::SerialRecv,"串口缓存未更新");
            times_noData = 0;
        }
        return result;
    }

    int pos = plotReadPos;

    for (int i = 0; i < available; ++i) {

        quint16 voltage = (static_cast<quint8>(buffer[pos].second[3])
                           << 8)
                          | static_cast<quint8>(buffer[pos].second[2]);

        result.append(QPair<double,double>(buffer[pos].first,static_cast<double>(voltage) / 1000.0));
        pos = (pos + 1) % capacity;
    }

    plotReadPos = (plotReadPos + available) % capacity;

    return result;
}

int DataBuffer::size(){
    return count;
}
