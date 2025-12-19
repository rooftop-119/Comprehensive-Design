#ifndef COMMANDER_H
#define COMMANDER_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include "entity/frametype.h"
#include "entity/errortype.h"
#include "util/framefactory.h"
#include "logger.h"

struct PendingCommand {
    FrameType type;
    int seq;
    qint64 sendTime;
    QTimer* timeoutTimer;
};

class Commander : public QObject {
    Q_OBJECT
public:
    explicit Commander(FrameFactory* factory, QObject* parent = nullptr);
    ~Commander();

    // 命令发送接口（返回命令序列号）
    int sendStop();
    int sendStartVoltage(int intervalMs);
    int sendStartTemp(int intervalMs);
    int sendStartBoth(int intervalMs);

public slots:
    // 接收应答帧
    void onAnswerReceived(const AnsFrame& ans);

signals:
    // 统一的命令结果信号
    void commandCompleted(FrameType cmdType, int seq, ErrorType error);
    void commandTimeout(FrameType cmdType, int seq);

    // 底层发送信号（连接到 SerialWorker）
    void sendData(const QByteArray& data);

    void log(Logger::Type type, const QString& msg, bool clear = false);

private slots:
    void onCommandTimeout();

private:
    int sendCommand(FrameType type, const QByteArray& data = QByteArray());
    void registerPendingCommand(FrameType type, int seq);
    void removePendingCommand(int seq);

    FrameFactory* m_factory;
    QMap<int, PendingCommand> m_pendingCommands;
    const int COMMAND_TIMEOUT_MS = 3000;
};

#endif
