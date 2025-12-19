#ifndef FILEWRITER_H
#define FILEWRITER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include "databuffer.h"
#include "logger.h"

class FileWriter : public QObject {
    Q_OBJECT
public:
    explicit FileWriter(DataBuffer *buffer, QObject *parent = nullptr);
    ~FileWriter();

    void rotateFileIfNeeded();

    void forceFlushNow();



    void stop();

    void start();

public slots:
    void flush();
    void update();

signals:
    void log(Logger::Type type, const QString &msg, bool clear=false);
    void fileNameUpdate();

private:
    QString generateFileName() ;

    QTimer* timer;
    DataBuffer *buf;
    QFile file;
    QTextStream stream;
    QString currentFileName;
};

#endif // FILEWRITER_H
