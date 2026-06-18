#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include "logger.h"

class DataBuffer : public QObject {
    Q_OBJECT
public:
    explicit DataBuffer(QObject *parent = nullptr);
    ~DataBuffer();

    void append(double timeSec, const QByteArray &data);   // 添加新数据
    QVector<QPair<double,QByteArray>> readForFile();
    QVector<QPair<double,double>> readForPlot(int num);
    int size();

signals:
    void flushToFile(QVector<QPair<double,QByteArray>> buffer);
    void noDataForPlot();
    void log(Logger::Type type, const QString &msg, bool clear=false);

private:
    QVector<QPair<double,QByteArray>> buffer;
    int capacity;

    int writePos;       // 写入位置
    int plotReadPos;    // 绘图读取位置
    int fileWritePos;   // 文件写入位置
    int count;          // 当前有效数据量

    mutable QMutex mutex;
};
#endif // DATABUFFER_H
