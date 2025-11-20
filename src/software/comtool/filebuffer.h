#ifndef FILEBUFFER_H
#define FILEBUFFER_H

#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include "logger.h"

class FileBuffer : public QObject {
    Q_OBJECT
public:
    explicit FileBuffer(QObject *parent = nullptr);

    void append(const QStringList& lines);   // 添加新数据
    QVector<QPair<double,double>> readForPlot(int num);
    int size();

signals:
    void noDataForPlot();
    void log(Logger::Type type, const QString &msg, bool clear=false);

private:
    QVector<QPair<double,double>> buffer;
    int capacity;

    int writePos;       // 写入位置
    int plotReadPos;    // 绘图读取位置
    int count;          // 当前有效数据量
    mutable QMutex mutex;
};

#endif // FILEBUFFER_H
