#include "filereader.h"
#include <QThread>
#include "QCoreApplication"

FileReader::FileReader(QObject* parent)
    : QObject(parent),
    in(&file)                      // 使用成员 QTextStream
{
    timer = new QTimer(this);        // parent = this，防止悬空
    timer->setInterval(interval);
    timer->setSingleShot(false);
    connect(timer, &QTimer::timeout, this, &FileReader::onTimeout);
    baseDir = QCoreApplication::applicationDirPath() + "/logs/";
}

void FileReader::start(const QString& filename, int intervalMs)
{
    // 如果调用发生在非对象所属线程，转发到对象线程（安全）
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "start",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, filename),
                                  Q_ARG(int, intervalMs));
        return;
    }

    // 保护：timer 可能为 nullptr（未正确构造或已被销毁）
    if (!timer) {
        qWarning() << "FileReader::start - timer is null, creating with parent=this";
        timer = new QTimer(this);
        timer->setInterval(interval);
        timer->setSingleShot(false);
        connect(timer, &QTimer::timeout, this, &FileReader::onTimeout);
    }

    // 安全停止定时器
    if (timer->isActive()) timer->stop();

    // 安全地重置文件
    if (file.isOpen()) file.close();
    file.setFileName(baseDir+filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit log(Logger::Type::File, QStringLiteral("无法打开文件: %1").arg(filename));
        return;
    }

    // 若你有成员 QTextStream in; 需要绑定到 file（推荐）
    in.setDevice(&file);
    in.seek(0); // 可选：从文件头开始

    timer->start(intervalMs);
    qDebug() << "FileReader::start - started timer with interval" << intervalMs;
}

void FileReader::stop()
{
    if (timer->isActive())
        timer->stop();

    file.close();
}

void FileReader::onTimeout()
{
    if (!file.isOpen())
        return;

    QStringList chunk;

    for (int i = 0; i < linesPerTick; ++i) {
        if (in.atEnd()) {
            emit fileReadOver();
            stop();
            return;
        }
        chunk << in.readLine();
    }

    emit linesRead(chunk);
}
