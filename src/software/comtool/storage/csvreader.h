#ifndef CSVREADER_H
#define CSVREADER_H

#include "istorage.h"
#include <QFile>
#include <QTextStream>

class CSVReader : public IStorage {
    Q_OBJECT
public:
    explicit CSVReader(QObject* parent = nullptr);
    ~CSVReader() override;

    // 配置接口
    void setFilePath(const QString& path);

    // IStorage 接口实现
    bool open() override;
    void close() override;
    bool isOpen() const override;

    bool write(const Sample& sample) override { Q_UNUSED(sample); return false; }
    int writeBatch(const QVector<Sample>& samples) override { Q_UNUSED(samples); return 0; }
    void flush() override {}

    Sample readOne() override;
    QVector<Sample> readBatch(int count) override;
    QVector<Sample> readAll() override;

    qint64 recordCount() const override { return m_totalRecords; }
    qint64 fileSize() const override;
    QString filePath() const override { return m_filePath; }

    // 导航操作
    bool seek(qint64 recordIndex);
    bool seekToTime(double timestamp);
    void reset();

    // 查询接口
    qint64 currentPosition() const { return m_currentRecord; }
    bool atEnd() const;
    double startTime() const { return m_startTime; }
    double endTime() const { return m_endTime; }

signals:
    void fileLoaded(qint64 records, double duration);

private:
    Sample parseLine(const QString& line, bool* ok = nullptr);
    void analyzeFile();

    QString m_filePath;
    QFile m_file;
    QTextStream m_stream;

    qint64 m_totalRecords = 0;
    qint64 m_currentRecord = 0;
    double m_startTime = 0.0;
    double m_endTime = 0.0;
};

#endif
