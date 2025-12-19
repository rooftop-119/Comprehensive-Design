#include "databuffer.h"

DataBuffer::DataBuffer(QObject* parent)
    : QObject(parent),
    m_writePos(0),
    m_visualizerReadPos(0),
    m_writerReadPos(0),
    m_size(0),
    m_overflowCount(0)
{
    m_buffer.resize(m_capacity);
    emit log(Logger::Type::System,
             QString("DataBuffer 初始化，容量: %1").arg(m_capacity));
}

DataBuffer::~DataBuffer() {
    if (m_overflowCount > 0) {
        emit log(Logger::Type::System,
                 QString("DataBuffer 销毁，总溢出次数: %1").arg(m_overflowCount));
    }
}

void DataBuffer::setEnableStorage(bool enableStorage){
    m_enableStorage = enableStorage;
}

bool DataBuffer::write(const Sample& sample) {
    QMutexLocker locker(&m_mutex);

    // 检查是否满（考虑两个读指针中最慢的那个）
    int slowestReadPos = qMin(m_visualizerReadPos, m_writerReadPos);
    int usedSpace = (m_writePos - slowestReadPos + m_capacity) % m_capacity;

    if (usedSpace >= m_capacity - 1) {  // 保留一个位置区分满和空
        // 缓冲区满，推进最慢的读指针
        if (m_visualizerReadPos == slowestReadPos) {
            m_visualizerReadPos = (m_visualizerReadPos + 1) % m_capacity;
        }
        if (m_writerReadPos == slowestReadPos) {
            m_writerReadPos = (m_writerReadPos + 1) % m_capacity;
        }

        m_overflowCount++;
        emit bufferOverflow(1);
        emit log(Logger::Type::System,
                 QString("缓冲区溢出，丢失数据 seq=%1").arg(sample.seq));
    }

    m_buffer[m_writePos] = sample;
    m_writePos = (m_writePos + 1) % m_capacity;

    // 更新大小（计算最慢读指针到写指针的距离）
    slowestReadPos = qMin(m_visualizerReadPos, m_writerReadPos);
    int currentSize = (m_writePos - slowestReadPos + m_capacity) % m_capacity;
    m_size.storeRelease(currentSize);

    emit dataAvailable(currentSize);
    return true;
}

int DataBuffer::writeBatch(const QVector<Sample>& samples) {
    if (samples.isEmpty()) return 0;

    QMutexLocker locker(&m_mutex);

    int written = 0;
    int overflow = 0;

    for (const auto& sample : samples) {
        // 检查是否满
        int slowestReadPos = qMin(m_visualizerReadPos, m_writerReadPos);
        int usedSpace = (m_writePos - slowestReadPos + m_capacity) % m_capacity;

        if (usedSpace >= m_capacity - 1) {
            // 推进最慢的读指针
            if (m_visualizerReadPos == slowestReadPos) {
                m_visualizerReadPos = (m_visualizerReadPos + 1) % m_capacity;
            }
            if (m_writerReadPos == slowestReadPos) {
                m_writerReadPos = (m_writerReadPos + 1) % m_capacity;
            }
            overflow++;
        }

        m_buffer[m_writePos] = sample;
        m_writePos = (m_writePos + 1) % m_capacity;
        written++;
    }

    if (overflow > 0) {
        m_overflowCount += overflow;
        emit bufferOverflow(overflow);
        emit log(Logger::Type::System,
                 QString("批量写入溢出，丢失 %1 条数据").arg(overflow));
    }

    // 更新大小
    int slowestReadPos = qMin(m_visualizerReadPos, m_writerReadPos);
    int currentSize = (m_writePos - slowestReadPos + m_capacity) % m_capacity;
    m_size.storeRelease(currentSize);

    emit dataAvailable(currentSize);
    return written;
}

QVector<Sample> DataBuffer::readForVisualize(int maxCount) {
    QMutexLocker locker(&m_mutex);

    QVector<Sample> result;

    // 计算可读数量
    int available = (m_writePos - m_visualizerReadPos + m_capacity) % m_capacity;
    int toRead = qMin(available, maxCount);

    if (toRead == 0) {
        return result;
    }

    result.reserve(toRead);

    // 读取数据
    for (int i = 0; i < toRead; ++i) {
        int pos = (m_visualizerReadPos + i) % m_capacity;
        result.append(m_buffer[pos]);
    }

    // 推进读指针
    m_visualizerReadPos = (m_visualizerReadPos + toRead) % m_capacity;
    if(!m_enableStorage){
        m_writerReadPos = m_visualizerReadPos;
    }

    // 更新大小（基于最慢的读指针）
    int currentSize = (m_writePos - m_visualizerReadPos + m_capacity) % m_capacity;
    m_size.storeRelease(currentSize);

    return result;
}

QVector<Sample> DataBuffer::readForStorage() {
    QMutexLocker locker(&m_mutex);

    QVector<Sample> result;

    // 计算可读数量
    int available = (m_writePos - m_writerReadPos + m_capacity) % m_capacity;

    if (available == 0) {
        return result;
    }

    result.reserve(available);

    // 读取所有可用数据
    for (int i = 0; i < available; ++i) {
        int pos = (m_writerReadPos + i) % m_capacity;
        result.append(m_buffer[pos]);
    }

    // 推进读指针
    m_writerReadPos = (m_writerReadPos + available) % m_capacity;

    // 更新大小（基于最慢的读指针）
    int slowestReadPos = qMin(m_visualizerReadPos, m_writerReadPos);
    int currentSize = (m_writePos - slowestReadPos + m_capacity) % m_capacity;
    m_size.storeRelease(currentSize);

    return result;
}

int DataBuffer::size() const {
    return m_size.loadAcquire();
}

void DataBuffer::clear() {
    QMutexLocker locker(&m_mutex);

    m_writePos = 0;
    m_visualizerReadPos = 0;
    m_writerReadPos = 0;
    m_size.storeRelease(0);

    emit log(Logger::Type::System, "DataBuffer 已清空");
}

void DataBuffer::resize(int newCapacity) {
    QMutexLocker locker(&m_mutex);

    if (newCapacity <= 0) return;

    QVector<Sample> newBuffer;
    newBuffer.resize(newCapacity);

    // 从最慢的读指针开始复制数据
    int slowestReadPos = qMin(m_visualizerReadPos, m_writerReadPos);
    int available = (m_writePos - slowestReadPos + m_capacity) % m_capacity;
    int copyCount = qMin(available, newCapacity - 1);  // 保留一个位置

    for (int i = 0; i < copyCount; ++i) {
        int oldPos = (slowestReadPos + i) % m_capacity;
        newBuffer[i] = m_buffer[oldPos];
    }

    m_buffer = newBuffer;
    m_capacity = newCapacity;

    // 重置指针
    m_writePos = copyCount;
    m_visualizerReadPos = 0;
    m_writerReadPos = 0;
    m_size.storeRelease(copyCount);

    emit log(Logger::Type::System,
             QString("DataBuffer 容量调整为: %1，保留数据: %2 条")
                 .arg(newCapacity)
                 .arg(copyCount));
}
