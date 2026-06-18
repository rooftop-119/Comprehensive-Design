#include "csvreader.h"
#include <QFileInfo>

CSVReader::CSVReader(QObject* parent)
    : IStorage(parent)
{
}

CSVReader::~CSVReader() {
    close();
}

void CSVReader::setFilePath(const QString& path) {
    if (isOpen()) {
        emit log(Logger::Type::File, "警告：文件路径将在下次打开时生效");
    }
    m_filePath = path;
}

bool CSVReader::open() {
    if (m_filePath.isEmpty()) {
        emit log(Logger::Type::File, "错误：文件路径为空");
        emit errorOccurred("文件路径为空");
        return false;
    }

    m_file.setFileName(m_filePath);

    if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString error = QString("无法打开文件: %1 - %2")
                            .arg(m_filePath)
                            .arg(m_file.errorString());
        emit log(Logger::Type::File, error);
        emit errorOccurred(error);
        return false;
    }

    m_stream.setDevice(&m_file);
    m_stream.setEncoding(QStringConverter::Utf8);

    // 跳过表头
    if (!m_stream.atEnd()) {
        m_stream.readLine();
    }

    analyzeFile();

    emit log(Logger::Type::File,
             QString("CSV文件已打开: %1 (共 %2 条记录)")
                 .arg(m_filePath)
                 .arg(m_totalRecords));

    return true;
}

void CSVReader::close() {
    if (m_file.isOpen()) {
        m_file.close();
    }

    m_totalRecords = 0;
    m_currentRecord = 0;
    m_startTime = 0.0;
    m_endTime = 0.0;
}

bool CSVReader::isOpen() const {
    return m_file.isOpen();
}

Sample CSVReader::readOne() {
    if (!isOpen() || atEnd()) {
        return Sample{};
    }

    QString line = m_stream.readLine();
    bool ok;
    Sample sample = parseLine(line, &ok);

    if (ok) {
        m_currentRecord++;
        emit dataRead(1);
    }

    return sample;
}

QVector<Sample> CSVReader::readBatch(int count) {
    QVector<Sample> result;
    result.reserve(count);

    for (int i = 0; i < count && !atEnd(); ++i) {
        bool ok;
        Sample sample = readOne();
        if (sample.timestamp > 0) {
            result.append(sample);
        }
    }

    return result;
}

QVector<Sample> CSVReader::readAll() {
    reset();

    QVector<Sample> result;
    result.reserve(m_totalRecords);

    while (!atEnd()) {
        Sample sample = readOne();
        if (sample.timestamp > 0) {
            result.append(sample);
        }
    }

    emit log(Logger::Type::File,
             QString("已读取全部数据: %1 条记录").arg(result.size()));

    return result;
}

qint64 CSVReader::fileSize() const {
    return m_file.size();
}

bool CSVReader::seek(qint64 recordIndex) {
    if (recordIndex < 0 || recordIndex >= m_totalRecords) {
        return false;
    }

    reset();

    // 跳过指定数量的记录
    for (qint64 i = 0; i < recordIndex && !atEnd(); ++i) {
        m_stream.readLine();
    }

    m_currentRecord = recordIndex;
    return true;
}

bool CSVReader::seekToTime(double timestamp) {
    if (timestamp < m_startTime || timestamp > m_endTime) {
        return false;
    }

    reset();

    while (!atEnd()) {
        qint64 pos = m_stream.pos();
        QString line = m_stream.readLine();

        bool ok;
        Sample sample = parseLine(line, &ok);

        if (ok && sample.timestamp >= timestamp) {
            m_stream.seek(pos);
            return true;
        }

        m_currentRecord++;
    }

    return false;
}

void CSVReader::reset() {
    m_stream.seek(0);
    m_currentRecord = 0;

    // 跳过表头
    if (!m_stream.atEnd()) {
        m_stream.readLine();
    }
}

bool CSVReader::atEnd() const {
    return m_stream.atEnd();
}

Sample CSVReader::parseLine(const QString& line, bool* ok) {
    Sample sample{};

    if (ok) *ok = false;

    QStringList parts = line.split(',');
    if (parts.size() < 5) return sample;

    bool timestampOk, seqOk, voltOk, tempOk;

    sample.timestamp = parts[0].toDouble(&timestampOk);
    sample.seq = parts[2].toInt(&seqOk);

    // 电压字段（可能为空）
    if (!parts[3].isEmpty()) {
        sample.voltage = parts[3].toDouble(&voltOk);
        sample.hasVoltage = voltOk;
    }

    // 温度字段（可能为空）
    if (!parts[4].isEmpty()) {
        sample.temperature = parts[4].toDouble(&tempOk);
        sample.hasTemperature = tempOk;
    }

    if (ok) {
        *ok = timestampOk && seqOk;
    }

    return sample;
}

void CSVReader::analyzeFile() {
    qint64 originalPos = m_stream.pos();

    m_totalRecords = 0;
    m_startTime = 0.0;
    m_endTime = 0.0;

    reset();

    bool firstRecord = true;

    while (!m_stream.atEnd()) {
        QString line = m_stream.readLine();

        bool ok;
        Sample sample = parseLine(line, &ok);

        if (ok) {
            m_totalRecords++;

            if (firstRecord) {
                m_startTime = sample.timestamp;
                firstRecord = false;
            }

            m_endTime = sample.timestamp;
        }
    }

    // 恢复位置
    m_stream.seek(originalPos);

    double duration = m_endTime - m_startTime;
    emit fileLoaded(m_totalRecords, duration);
    emit log(Logger::Type::File,
             QString("文件分析完成: %1 条记录, 时长 %2 秒")
                 .arg(m_totalRecords)
                 .arg(duration, 0, 'f', 3));
}
