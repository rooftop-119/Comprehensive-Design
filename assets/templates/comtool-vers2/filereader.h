#ifndef FILEREADER_H
#define FILEREADER_H

#include <QObject>
#include <QFile>
#include <QTimer>
#include "logger.h"

class FileReader : public QObject
{
    Q_OBJECT
public:
    explicit FileReader(QObject* parent = nullptr);

public slots:
    void start(const QString& filename, int intervalMs);
    void stop();

signals:
    void linesRead(const QStringList& lines);  // 返回一批数据
    void fileReadOver();                       // 文件读完
    void log(Logger::Type type, const QString &msg, bool clear=false);

private slots:
    void onTimeout();                          // 定时读取

private:
    QFile file;
    QTimer* timer;
    QTextStream in;
    int interval = 250;
    int linesPerTick = 100;
    QString baseDir;
};

#endif // FILEREADER_H
