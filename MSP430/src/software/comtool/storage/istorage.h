#ifndef ISTORAGE_H
#define ISTORAGE_H

#include <QObject>
#include <memory>
#include "logger.h"

class IStorage : public QObject {
    Q_OBJECT
public:

    explicit IStorage(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IStorage() = default;

    // 基本操作
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // 数据传输
    virtual bool write(const Sample& sample) = 0;
    virtual int writeBatch(const QVector<Sample>& samples) = 0;
    virtual void flush() = 0;

    // IStorage 接口实现
    virtual Sample readOne() = 0;
    virtual QVector<Sample> readBatch(int count) = 0;
    virtual QVector<Sample> readAll() = 0;

    virtual qint64 recordCount() const = 0;
    virtual qint64 fileSize() const = 0;
    virtual QString filePath() const = 0;

signals:
    void errorOccurred(const QString& error);
    void log(Logger::Type type, const QString& msg, bool clear = false);
    void dataRead(int);
    void dataWritten(int);
};

#endif // ISTORAGE_H
