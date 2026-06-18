#include "filewriter.h"

FileWriter::FileWriter(DataBuffer *buffer, QObject *parent)
    : QObject(parent),
    buf(buffer)
{
    QDir().mkpath("logs");

    currentFileName = generateFileName();

    // 先 new Timer 再 connect
    timer = new QTimer(this);
    timer->setInterval(5000);
    connect(timer, &QTimer::timeout, this, &FileWriter::flush);

    emit log(Logger::Type::File,
             QString("FileWriter initing，当前写入文件：%1")
                 .arg(currentFileName));
}

FileWriter::~FileWriter() {
    stop();
    if (file.isOpen()) {
        stream.flush();
        file.close();
    }
}

void FileWriter::start() {
    timer->start();
}

void FileWriter::stop() {
    timer->stop();
}

void FileWriter::flush() {
    if (!buf || buf->size() == 0) return;

    QVector<QPair<double,QByteArray>> data = buf->readForFile();
    if (data.isEmpty()) return;

    // 打开文件（追加）
    if (!file.isOpen()) {
        file.setFileName(currentFileName);
        if (!file.open(QIODevice::Append | QIODevice::Text)) {
            emit log(Logger::Type::File,
                     "文件写入失败: " + file.errorString());
            return;
        }
        stream.setDevice(&file);
    }

    for (const auto &p : data) {
        stream << QString("%1,%2,%3,%4,%5,%6\n")
        .arg(p.first, 0, 'f', 3)
            .arg(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(p.first))
                     .toString("yyyy-MM-dd hh:mm:ss.zzz"))
            .arg(static_cast<int>(static_cast<quint8>(p.second[0])), 0, 16)
            .arg(static_cast<int>(static_cast<quint8>(p.second[1])), 0, 16)
            .arg(double((static_cast<quint8>(p.second[3]) << 8)
                        | static_cast<quint8>(p.second[2])) / 1000.0,
                 0, 'f', 4)
            .arg(static_cast<int>(static_cast<quint8>(p.second[4])), 0, 16);
    }

    stream.flush();
    rotateFileIfNeeded();
}

void FileWriter::rotateFileIfNeeded() {
    if (!file.isOpen()) return;

    if (file.size() > 5 * 1024 * 1024) { // 5MB
        file.close();
        update();
    }
}

void FileWriter::update() {
    currentFileName = generateFileName();
    emit log(Logger::Type::File,
             QString("文件存储更新：%1").arg(currentFileName));
    emit fileNameUpdate();
}

QString FileWriter::generateFileName() {
    QString time = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("logs/data_%1.csv").arg(time);
}
