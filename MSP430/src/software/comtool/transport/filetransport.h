#ifndef FILETRANSPORT_H
#define FILETRANSPORT_H

#include "itransport.h"
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include "api/appconfig.h"

class FileTransport : public ITransport {
    Q_OBJECT
public:
    explicit FileTransport(QObject* parent = nullptr);
    ~FileTransport() override;

    // 配置接口
    void setFilePath(const QString& path);
    void setPlaybackSpeed(double speed) { m_playbackSpeed = speed; }
    void setLoopMode(bool loop) { m_loopMode = loop; }

    // ITransport 接口实现
    bool open() override;
    void close() override;
    bool isOpen() const override;
    State state() const override { return m_state; }

    bool write(const QByteArray& data) override {
        Q_UNUSED(data);
        return false;  // 文件传输不支持写入
    }

    qint64 bytesReceived() const override { return m_samplesRead; }
    qint64 bytesSent() const override { return 0; }

    // 播放控制
    void pause();
    void resume();
    void seek(double timestamp);

signals:
    void playbackProgress(double currentTime, double totalTime);
    void playbackFinished();

private slots:
    void onTimerTimeout();

private:
    void readNextBatch();
    Sample parseLine(const QString& line);

    QString m_filePath;
    QFile m_file;
    QTextStream m_stream;
    QTimer* m_timer = nullptr;

    State m_state = State::Disconnected;
    double m_playbackSpeed = 1.0;
    bool m_loopMode = false;

    qint64 m_samplesRead = 0;
    double m_currentTime = 0.0;
    double m_totalTime = 0.0;

    static int BATCH_SIZE;
    static int TIMER_INTERVAL;
};

#endif
