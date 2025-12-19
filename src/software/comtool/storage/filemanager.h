#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QFileInfo>
#include "logger.h"

struct FileInfo {
    QString fileName;
    QString filePath;
    qint64 fileSize;
    QDateTime lastModified;
    qint64 recordCount;
};

class FileManager : public QObject {
    Q_OBJECT
public:
    explicit FileManager(QObject* parent = nullptr);

    // 文件查询
    QList<FileInfo> getRecentFiles(const QString& dir, int count = 10);
    QStringList getFileNames(const QString& dir, const QString& pattern = "*.csv");

    // 文件操作
    bool deleteFile(const QString& filePath);
    bool renameFile(const QString& oldPath, const QString& newPath);
    qint64 getTotalSize(const QString& dir);
    int getFileCount(const QString& dir);

    // 文件分析
    FileInfo analyzeFile(const QString& filePath);
    qint64 countRecords(const QString& filePath);

    // 清理操作
    int cleanOldFiles(const QString& dir, int daysOld);
    int cleanBySize(const QString& dir, qint64 maxTotalSize);

signals:
    void log(Logger::Type type, const QString& msg, bool clear = false);

private:
    qint64 quickCountLines(const QString& filePath);
};

#endif
