#include "csvwriter.h"
#include <QDir>
#include <QDateTime>

CSVWriter::CSVWriter(QObject* parent)
    : IStorage(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(m_flushInterval);
    connect(m_timer, &QTimer::timeout, this, &CSVWriter::onFlushTimeout);

    // 确保输出目录存在
    QDir().mkpath(m_outputDir);
}

CSVWriter::~CSVWriter() {
    stop();
    closeCurrentFile();
}

void CSVWriter::setDataSource(DataBuffer* buffer) {
    m_dataSource = buffer;
}

void CSVWriter::setOutputDir(const QString& dir) {
    if (m_isRunning) {
        emit log(Logger::Type::File, "警告：输出目录将在下次启动时生效");
    }
    m_outputDir = dir;
    QDir().mkpath(m_outputDir);
}

void CSVWriter::setFlushInterval(int ms) {
    m_flushInterval = ms;
    if (m_timer) {
        m_timer->setInterval(ms);
    }
}

void CSVWriter::setMaxFileSize(qint64 bytes) {
    m_maxFileSize = bytes;
}

bool CSVWriter::open() {
    openNewFile();
    return isOpen();
}

void CSVWriter::close() {
    closeCurrentFile();
}

bool CSVWriter::isOpen() const {
    return m_file.isOpen();
}

bool CSVWriter::write(const Sample& sample) {
    if (!isOpen()) {
        emit log(Logger::Type::File, "文件未打开，无法写入");
        return false;
    }

    writeSample(sample);
    emit dataWritten(1);

    if (m_autoRotate) {
        rotateFileIfNeeded();
    }

    return true;
}

int CSVWriter::writeBatch(const QVector<Sample>& samples) {
    if (!isOpen() || samples.isEmpty()) return 0;

    for (const auto& sample : samples) {
        writeSample(sample);
    }

    emit dataWritten(samples.size());

    if (m_autoRotate) {
        rotateFileIfNeeded();
    }

    return samples.size();
}

void CSVWriter::flush() {
    if (!m_dataSource || !isOpen()) return;

    // 从数据源读取所有待写入数据
    QVector<Sample> samples = m_dataSource->readForStorage();
    if (samples.isEmpty()) return;

    writeBatch(samples);
    m_stream.flush();

    emit log(Logger::Type::File,
             QString("已刷新 %1 条数据到文件").arg(samples.size()));
}

qint64 CSVWriter::fileSize() const {
    return m_file.size();
}

void CSVWriter::start() {
    if (m_isRunning) {
        emit log(Logger::Type::File, "CSV写入已在运行中");
        return;
    }

    if (!m_dataSource) {
        emit log(Logger::Type::File, "错误：未设置数据源");
        emit errorOccurred("未设置数据源");
        return;
    }

    openNewFile();

    m_isRunning = true;
    m_timer->start();

    emit log(Logger::Type::File,
             QString("CSV写入已启动: %1").arg(m_currentFile));
}

void CSVWriter::stop() {
    if (!m_isRunning) return;

    m_timer->stop();
    flush();  // 最后一次刷新
    closeCurrentFile();
    m_isRunning = false;

    emit log(Logger::Type::File,
             QString("CSV写入已停止，共写入 %1 条记录").arg(m_samplesWritten));
}

void CSVWriter::forceRotate() {
    if (!isOpen()) return;

    emit log(Logger::Type::File, "手动触发文件切换");
    openNewFile();
}

void CSVWriter::onFlushTimeout() {
    flush();
}

void CSVWriter::openNewFile() {
    // 关闭当前文件
    closeCurrentFile();

    // 生成新文件名
    if(startTimes == 0){
        m_currentFile = generateFileName();
        startTimes = (startTimes + 1)% 5;
    }
    m_file.setFileName(m_currentFile);

    // 打开文件
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QString error = QString("无法创建文件: %1 - %2")
                            .arg(m_currentFile)
                            .arg(m_file.errorString());
        emit log(Logger::Type::File, error);
        emit errorOccurred(error);
        return;
    }

    m_stream.setDevice(&m_file);
    m_stream.setEncoding(QStringConverter::Utf8); // 确保UTF-8编码

    // 如果是新文件，写入表头
    if (m_file.size() == 0) {
        writeHeader();
    }

    emit fileRotated(m_currentFile);
    emit log(Logger::Type::File,
             QString("新CSV文件已创建: %1").arg(m_currentFile));
}

void CSVWriter::closeCurrentFile() {
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
}

void CSVWriter::writeHeader() {
    m_stream << "timestamp,datetime,seq,voltage,temperature\n";
    m_stream.flush();
}

void CSVWriter::writeSample(const Sample& sample) {
    // 生成可读的日期时间字符串
    QString datetime = QDateTime::fromMSecsSinceEpoch(
                           static_cast<qint64>(sample.timestamp * 1000)
                           ).toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 写入CSV行
    m_stream << QString("%1,%2,%3,%4,%5\n")
                    .arg(sample.timestamp, 0, 'f', 3)
                    .arg(datetime)
                    .arg(sample.seq)
                    .arg(sample.hasVoltage ? QString::number(sample.voltage, 'f', 3) : "")
                    .arg(sample.hasTemperature ? QString::number(sample.temperature, 'f', 2) : "");

    m_samplesWritten++;
}

void CSVWriter::rotateFileIfNeeded() {
    if (!m_autoRotate) return;

    if (m_file.size() >= m_maxFileSize) {
        emit log(Logger::Type::File,
                 QString("文件大小超限(%1 MB)，自动切换文件")
                     .arg(m_file.size() / 1024.0 / 1024.0, 0, 'f', 2));
        openNewFile();
    }
}

QString CSVWriter::generateFileName() const {
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1/data_%2.csv").arg(m_outputDir).arg(timestamp);
}
