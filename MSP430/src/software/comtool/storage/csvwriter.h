#ifndef CSVWRITER_H
#define CSVWRITER_H

#include <QFile>
#include <QTextStream>
#include <QTimer>
#include "core/databuffer.h"
#include "istorage.h"

class CSVWriter  : public IStorage{
    Q_OBJECT
public:
    explicit CSVWriter(QObject* parent = nullptr);
    ~CSVWriter();

    // 配置接口
    void setDataSource(DataBuffer* buffer);
    void setOutputDir(const QString& dir);
    void setFlushInterval(int ms);
    void setMaxFileSize(qint64 bytes);
    void setAutoRotate(bool enable) { m_autoRotate = enable; }

    // IStorage 接口实现
    bool open() override;
    void close() override;
    bool isOpen() const override;

    bool write(const Sample& sample) override;
    int writeBatch(const QVector<Sample>& samples) override;
    void flush() override;

    Sample readOne() override { return Sample{}; }  // 写入器不支持读取
    QVector<Sample> readBatch(int count) override { Q_UNUSED(count); return {}; }
    QVector<Sample> readAll() override { return {}; }

    qint64 recordCount() const override { return m_samplesWritten; }
    qint64 fileSize() const override;
    QString filePath() const override { return m_currentFile; }

    // 控制接口
    void start();
    void stop();
    void forceRotate();

    // 查询接口
    QString currentFileName() const { return m_currentFile; }
    qint64 samplesWritten() const { return m_samplesWritten; }
    bool isAutoRotating() const { return m_autoRotate; }

signals:
    void fileRotated(const QString& newFile);

private slots:
    void onFlushTimeout();

private:
    void openNewFile();
    void closeCurrentFile();
    void writeHeader();
    void writeSample(const Sample& sample);
    void rotateFileIfNeeded();
    QString generateFileName() const;

    DataBuffer* m_dataSource = nullptr;
    QTimer* m_timer = nullptr;

    QString m_outputDir = "logs";
    QString m_currentFile;
    QFile m_file;
    QTextStream m_stream;

    int m_flushInterval = 5000;  // ms
    qint64 m_maxFileSize = 1024 * 1024;  // 1MB
    qint64 m_samplesWritten = 0;
    bool m_autoRotate = true;
    bool m_isRunning = false;

    int startTimes = 0;
};

#endif
