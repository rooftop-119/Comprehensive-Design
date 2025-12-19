#include "commander.h"
#include <QDateTime>

Commander::Commander(FrameFactory* factory, QObject* parent)
    : QObject(parent), m_factory(factory) {
}

Commander::~Commander() {
    // 清理所有待处理的定时器
    for (auto& cmd : m_pendingCommands) {
        if (cmd.timeoutTimer) {
            cmd.timeoutTimer->stop();
            cmd.timeoutTimer->deleteLater();
        }
    }
    m_pendingCommands.clear();
}

int Commander::sendStop() {
    return sendCommand(FrameType::CmdStop);
}

int Commander::sendStartVoltage(int intervalMs) {
    QByteArray data = QByteArray::fromHex(
        QString("%1").arg(intervalMs/5, 2, 16, QChar('0')).toLatin1()
        );
    return sendCommand(FrameType::CmdStartVoltage, data);
}

int Commander::sendStartTemp(int intervalMs) {
    QByteArray data = QByteArray::fromHex(
        QString("%1").arg(intervalMs/5, 2, 16, QChar('0')).toLatin1()
        );
    return sendCommand(FrameType::CmdStartTemp, data);
}

int Commander::sendStartBoth(int intervalMs) {
    QByteArray data = QByteArray::fromHex(
        QString("%1%1").arg(intervalMs/5, 2, 16, QChar('0')).toLatin1()
        );
    return sendCommand(FrameType::CmdStartBoth, data);
}

int Commander::sendCommand(FrameType type, const QByteArray& data) {
    QByteArray frame = m_factory->buildCommandFrame(type, data);
    int seq = m_factory->getLastSeq();

    registerPendingCommand(type, seq);
    emit sendData(frame);
    emit log(Logger::Type::SerialSend,
             QString("发送命令 [%1] seq=%2").arg(static_cast<int>(type)).arg(seq));

    return seq;
}

void Commander::registerPendingCommand(FrameType type, int seq) {
    PendingCommand cmd;
    cmd.type = type;
    cmd.seq = seq;
    cmd.sendTime = QDateTime::currentMSecsSinceEpoch();

    // 创建超时定时器
    cmd.timeoutTimer = new QTimer(this);
    cmd.timeoutTimer->setSingleShot(true);
    cmd.timeoutTimer->setInterval(COMMAND_TIMEOUT_MS);

    connect(cmd.timeoutTimer, &QTimer::timeout, this, [this, seq]() {
        onCommandTimeout();
    });

    cmd.timeoutTimer->setProperty("seq", seq);
    cmd.timeoutTimer->start();

    m_pendingCommands.insert(seq, cmd);
}

void Commander::onAnswerReceived(const AnsFrame& ans) {
    int seq = ans.seq();

    auto it = m_pendingCommands.find(seq);
    if (it == m_pendingCommands.end()) {
        emit log(Logger::Type::System,
                 QString("收到未知序列号的应答: seq=%1").arg(seq));
        return;
    }

    PendingCommand cmd = it.value();

    // 停止并删除定时器
    if (cmd.timeoutTimer) {
        cmd.timeoutTimer->stop();
        cmd.timeoutTimer->deleteLater();
    }

    m_pendingCommands.erase(it);

    // 发送完成信号
    emit commandCompleted(cmd.type, seq, ans.errorType());
    emit log(Logger::Type::System,
             QString("命令完成 [%1] seq=%2 error=%3")
                 .arg(static_cast<int>(cmd.type))
                 .arg(seq)
                 .arg(static_cast<int>(ans.errorType())));
}

void Commander::onCommandTimeout() {
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;

    int seq = timer->property("seq").toInt();

    auto it = m_pendingCommands.find(seq);
    if (it == m_pendingCommands.end()) return;

    PendingCommand cmd = it.value();
    m_pendingCommands.erase(it);

    emit commandTimeout(cmd.type, seq);
    emit log(Logger::Type::System,
             QString("命令超时 [%1] seq=%2")
                 .arg(static_cast<int>(cmd.type))
                 .arg(seq));

    timer->deleteLater();
}

void Commander::removePendingCommand(int seq) {
    auto it = m_pendingCommands.find(seq);
    if (it != m_pendingCommands.end()) {
        if (it->timeoutTimer) {
            it->timeoutTimer->stop();
            it->timeoutTimer->deleteLater();
        }
        m_pendingCommands.erase(it);
    }
}
