#include "filetransport.h"
#include "entity/databothframe.h"

int FileTransport::BATCH_SIZE = AppConfig::linesPerTick;
int FileTransport::TIMER_INTERVAL = AppConfig::reader_interval;

FileTransport::FileTransport(QObject* parent)
    : ITransport(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(TIMER_INTERVAL);
    connect(m_timer, &QTimer::timeout, this, &FileTransport::onTimerTimeout);
}

FileTransport::~FileTransport() {
    close();
}

void FileTransport::setFilePath(const QString& path) {
    if (isOpen()) {
        emit log(Logger::Type::System, "警告：文件已打开，路径将在下次打开时生效");
    }
    m_filePath = path;
}

bool FileTransport::open() {
    if (m_filePath.isEmpty()) {
        emit log(Logger::Type::File, "文件路径为空");
        return false;
    }

    m_file.setFileName(m_filePath);
    if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit log(Logger::Type::File, QString("文件打开失败: %1").arg(m_filePath));
        emit errorOccurred("文件打开失败");
        return false;
    }

    m_stream.setDevice(&m_file);
    m_samplesRead = 0;
    m_currentTime = 0.0;

    m_state = State::Connected;
    emit log(Logger::Type::File, QString("文件已打开: %1").arg(m_filePath));

    m_timer->start();
    return true;
}

void FileTransport::close() {
    m_timer->stop();

    if (m_file.isOpen()) {
        m_file.close();
    }

    m_state = State::Disconnected;
    emit log(Logger::Type::File, "文件已关闭");
}

bool FileTransport::isOpen() const {
    return m_file.isOpen();
}

void FileTransport::pause() {
    m_timer->stop();
    emit log(Logger::Type::File, "播放已暂停");
}

void FileTransport::resume() {
    if (isOpen()) {
        m_timer->start();
        emit log(Logger::Type::File, "播放已恢复");
    }
}

void FileTransport::seek(double timestamp) {
    if (!isOpen()) return;

    m_stream.seek(0);
    m_currentTime = 0.0;

    QString line;
    while (!m_stream.atEnd()) {
        qint64 pos = m_stream.pos();
        line = m_stream.readLine();

        Sample sample = parseLine(line);
        if (sample.timestamp >= timestamp) {
            m_stream.seek(pos);
            m_currentTime = sample.timestamp;
            break;
        }
    }

    emit log(Logger::Type::File, QString("跳转到时间: %1s").arg(timestamp, 0, 'f', 3));
}

void FileTransport::onTimerTimeout() {
    readNextBatch();
}

void FileTransport::readNextBatch() {
    if (!isOpen() || m_stream.atEnd()) {
        if (m_loopMode) {
            m_stream.seek(0);
            m_currentTime = 0.0;
            m_samplesRead = 0;
            emit log(Logger::Type::File, "循环播放：重新开始");
        } else {
            m_timer->stop();
            emit playbackFinished();
            emit log(Logger::Type::File, "播放完成");
        }
        return;
    }

    int count = 0;
    while (count < BATCH_SIZE && !m_stream.atEnd()) {
        QString line = m_stream.readLine();
        Sample sample = parseLine(line);

        if (sample.timestamp > 0) {
            // 创建对应的帧
            auto frame = std::make_shared<DataBothFrame>(
                sample.timestamp,
                sample.seq,
                sample.voltage,
                sample.temperature,
                false  // CRC暂时设为false（文件数据假定有效）
                );

            emit frameReceived(frame);

            m_samplesRead++;
            m_currentTime = sample.timestamp;
            count++;
        }
    }

    emit playbackProgress(m_currentTime, m_totalTime);
}

Sample FileTransport::parseLine(const QString& line) {
    Sample sample{};

    QStringList parts = line.split(',');
    if (parts.size() < 5) return sample;

    bool ok;
    sample.timestamp = parts[0].toDouble(&ok);
    if (!ok) return sample;

    sample.seq = parts[2].toInt(&ok);
    if (!ok) return sample;

    sample.voltage = parts[3].toDouble(&ok);
    sample.hasVoltage = ok && (sample.voltage != 0.0);

    sample.temperature = parts[4].toDouble(&ok);
    sample.hasTemperature = ok && (sample.temperature != 0.0);

    return sample;
}
