#include "filebuffer.h"
#include <QThread>

FileBuffer::FileBuffer(QObject *parent)
    : QObject(parent),
    capacity(3000),
    writePos(0),
    plotReadPos(0),
    count(0)
{
    buffer.resize(capacity);
    emit log(Logger::Type::System,QStringLiteral("SerialBuffer initing，容量/帧：%1")
                                       .arg(capacity));
}

void FileBuffer::append(const QStringList &lines)
{
    QMutexLocker locker(&mutex);

    for (const QString &line : lines) {
        auto parts = line.split(",");

        if (parts.size() < 6)
            continue;

        bool ok1 = false, ok2 = false;
        double t = parts[0].toDouble(&ok1);
        double v = parts[4].toDouble(&ok2);

        if (!ok1 || !ok2)
            continue;

        // ---- 写入环形缓冲区 ----
        buffer[writePos] = QPair<double, double>(t, v);
        writePos = (writePos + 1) % capacity;

        if (count < capacity) {
            count++;
        } else {
            // 覆盖模式下推进读指针
            plotReadPos = (plotReadPos + 1) % capacity;
            emit log(Logger::Type::System,
                     "文件缓存溢出，覆盖一帧数据");
        }
    }
}

QVector<QPair<double,double>> FileBuffer::readForPlot(int num){
    QMutexLocker locker(&mutex);
    QVector<QPair<double,double>> result;
    static int times_noData = 0;
    int available = std::min((writePos - plotReadPos + capacity) % capacity,num);
    if(available == 0){
        times_noData++;
        if(times_noData == 20){
            emit noDataForPlot();
            times_noData = 0;
        }
        return result;
    }

    int pos = plotReadPos;

    for (int i = 0; i < available; ++i) {
        result.append(buffer[pos]);
        pos = (pos + 1) % capacity;
    }

    plotReadPos = (plotReadPos + available) % capacity;
    count -= available;
    if (count < 0) count = 0;
    return result;
}

int FileBuffer::size(){
    QMutexLocker locker(&mutex);
    return count;
}

