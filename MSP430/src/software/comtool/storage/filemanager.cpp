#include "filemanager.h"
#include <QDir>
#include <QFile>
#include <algorithm>

FileManager::FileManager(QObject* parent)
    : QObject(parent)
{
}

QList<FileInfo> FileManager::getRecentFiles(const QString& dir, int count) {
    QList<FileInfo> result;
    QDir directory(dir);

    if (!directory.exists()) {
        emit log(Logger::Type::File, QString("目录不存在: %1").arg(dir));
        return result;
    }

    QStringList csvFiles = directory.entryList(
        QStringList() << "*.csv",
        QDir::Files | QDir::NoDotAndDotDot
        );

    if (csvFiles.isEmpty()) {
        return result;
    }

    // 收集文件信息
    for (const QString& fileName : csvFiles) {
        QString filePath = directory.filePath(fileName);
        QFileInfo info(filePath);

        FileInfo fileInfo;
        fileInfo.fileName = fileName;
        fileInfo.filePath = filePath;
        fileInfo.fileSize = info.size();
        fileInfo.lastModified = info.lastModified();
        fileInfo.recordCount = quickCountLines(filePath) - 1;  // 减去表头

        result.append(fileInfo);
    }

    // 按修改时间降序排序
    std::sort(result.begin(), result.end(),
              [](const FileInfo& a, const FileInfo& b) {
                  return a.lastModified > b.lastModified;
              });

    // 取前count个
    if (result.size() > count) {
        result = result.mid(0, count);
    }

    return result;
}

QStringList FileManager::getFileNames(const QString& dir, const QString& pattern) {
    QDir directory(dir);

    if (!directory.exists()) {
        return QStringList();
    }

    return directory.entryList(
        QStringList() << pattern,
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Time  // 按时间排序
        );
}

bool FileManager::deleteFile(const QString& filePath) {
    QFile file(filePath);

    if (!file.exists()) {
        emit log(Logger::Type::File, QString("文件不存在: %1").arg(filePath));
        return false;
    }

    if (file.remove()) {
        emit log(Logger::Type::File, QString("文件已删除: %1").arg(filePath));
        return true;
    }

    emit log(Logger::Type::File,
             QString("删除文件失败: %1 - %2").arg(filePath).arg(file.errorString()));
    return false;
}

bool FileManager::renameFile(const QString& oldPath, const QString& newPath) {
    QFile file(oldPath);

    if (!file.exists()) {
        emit log(Logger::Type::File, QString("源文件不存在: %1").arg(oldPath));
        return false;
    }

    if (file.rename(newPath)) {
        emit log(Logger::Type::File,
                 QString("文件已重命名: %1 -> %2").arg(oldPath).arg(newPath));
        return true;
    }

    emit log(Logger::Type::File,
             QString("重命名失败: %1").arg(file.errorString()));
    return false;
}

qint64 FileManager::getTotalSize(const QString& dir) {
    QDir directory(dir);
    qint64 total = 0;

    QFileInfoList files = directory.entryInfoList(
        QStringList() << "*.csv",
        QDir::Files | QDir::NoDotAndDotDot
        );

    for (const QFileInfo& info : files) {
        total += info.size();
    }

    return total;
}

int FileManager::getFileCount(const QString& dir) {
    QDir directory(dir);
    return directory.entryList(
                        QStringList() << "*.csv",
                        QDir::Files | QDir::NoDotAndDotDot
                        ).count();
}

FileInfo FileManager::analyzeFile(const QString& filePath) {
    FileInfo info;
    QFileInfo fileInfo(filePath);

    info.fileName = fileInfo.fileName();
    info.filePath = filePath;
    info.fileSize = fileInfo.size();
    info.lastModified = fileInfo.lastModified();
    info.recordCount = countRecords(filePath);

    return info;
}

qint64 FileManager::countRecords(const QString& filePath) {
    return quickCountLines(filePath) - 1;  // 减去表头
}

int FileManager::cleanOldFiles(const QString& dir, int daysOld) {
    QDir directory(dir);
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-daysOld);

    QFileInfoList files = directory.entryInfoList(
        QStringList() << "*.csv",
        QDir::Files | QDir::NoDotAndDotDot
        );

    int deleted = 0;

    for (const QFileInfo& info : files) {
        if (info.lastModified() < cutoffDate) {
            if (QFile::remove(info.filePath())) {
                deleted++;
            }
        }
    }

    emit log(Logger::Type::File,
             QString("清理完成: 删除 %1 个过期文件 (>%2天)")
                 .arg(deleted).arg(daysOld));

    return deleted;
}

int FileManager::cleanBySize(const QString& dir, qint64 maxTotalSize) {
    QDir directory(dir);

    QFileInfoList files = directory.entryInfoList(
        QStringList() << "*.csv",
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Time  // 最旧的在后面
        );

    qint64 currentSize = 0;
    for (const QFileInfo& info : files) {
        currentSize += info.size();
    }

    if (currentSize <= maxTotalSize) {
        return 0;
    }

    int deleted = 0;

    // 从最旧的文件开始删除
    for (int i = files.size() - 1; i >= 0 && currentSize > maxTotalSize; --i) {
        const QFileInfo& info = files[i];
        qint64 fileSize = info.size();

        if (QFile::remove(info.filePath())) {
            currentSize -= fileSize;
            deleted++;
        }
    }

    emit log(Logger::Type::File,
             QString("按大小清理完成: 删除 %1 个文件，释放 %2 MB")
                 .arg(deleted)
                 .arg((getTotalSize(dir) - currentSize) / 1024.0 / 1024.0, 0, 'f', 2));

    return deleted;
}

qint64 FileManager::quickCountLines(const QString& filePath) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    qint64 lines = 0;
    while (!file.atEnd()) {
        file.readLine();
        lines++;
    }

    file.close();
    return lines;
}
