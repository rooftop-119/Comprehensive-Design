#ifndef ITRANSPORT_H
#define ITRANSPORT_H

#include <QObject>
#include <memory>
#include "entity/baseframe.h"
#include "logger.h"

class ITransport : public QObject {
    Q_OBJECT
public:
    enum class State {
        Disconnected,
        Connecting,
        Connected,
        Error
    };
    Q_ENUM(State)

    explicit ITransport(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ITransport() = default;

    // 基本操作
    Q_INVOKABLE virtual bool open() = 0;
    Q_INVOKABLE virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual State state() const = 0;

    // 数据传输
    virtual bool write(const QByteArray& data) = 0;

    // 统计信息
    virtual qint64 bytesReceived() const = 0;
    virtual qint64 bytesSent() const = 0;

signals:
    void stateChanged(ITransport::State state);
    void frameReceived(std::shared_ptr<BaseFrame> frame);
    void errorOccurred(const QString& error);
    void log(Logger::Type type, const QString& msg, bool clear = false);
};

#endif
