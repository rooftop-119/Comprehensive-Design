#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <QObject>
#include <QVector>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include "logger.h"
#include "head.h"

class DataBuffer : public QObject {
    Q_OBJECT
public:
    explicit DataBuffer(QObject* parent = nullptr);
    ~DataBuffer();

    // 写入接口
    bool write(const Sample& sample);
    int writeBatch(const QVector<Sample>& samples);

    // 读取接口（非阻塞）
    QVector<Sample> readForVisualize(int maxCount);
    QVector<Sample> readForStorage();
    QVector<Sample> readAll();

    // 查询接口
    int size() const;
    int capacity() const { return m_capacity; }
    bool isEmpty() const { return m_size.loadAcquire() == 0; }
    bool isFull() const { return m_size.loadAcquire() >= m_capacity; }
    void setEnableStorage(bool enableStorage);

    // 控制接口
    void clear();
    void resize(int newCapacity);

signals:
    void dataAvailable(int count);
    void bufferOverflow(int lostCount);
    void log(Logger::Type type, const QString& msg, bool clear = false);

private:
    QVector<Sample> m_buffer;
    int m_capacity = AppConfig::buffer_capacity;
    int m_writePos;
    int m_visualizerReadPos;  // 新增：可视化器读指针
    int m_writerReadPos;      // 新增：写入器读指针
    QAtomicInt m_size;
    bool m_enableStorage = true;

    mutable QMutex m_mutex;
    int m_overflowCount;
};

#endif // DATABUFFER_H
