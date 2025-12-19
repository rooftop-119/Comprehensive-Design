#include "datamanager.h"

DataManager::DataManager(QObject* parent)
    : QObject(parent)
{
    // 创建数据缓冲区
    m_buffer = new DataBuffer(this);

    connect(m_buffer, &DataBuffer::log, this, &DataManager::log);
    connect(m_buffer, &DataBuffer::bufferOverflow,
            this, [this](int lostCount) {
                emit log(Logger::Type::System,
                         QString("警告：数据溢出，丢失 %1 条数据").arg(lostCount));
            });
}

DataManager::~DataManager() {
    stopRecording();
}

void DataManager::initialize(QCustomPlot* plot,
                             QLCDNumber* tLcd, QLCDNumber* vLcd,
                             QDial* tDial, QDial* vDial)
{
    // 创建可视化器
    m_visualizer = new DataVisualizer(plot, this);
    m_visualizer->setDataSource(m_buffer);
    m_visualizer->setLCDDisplays(vLcd, tLcd);
    m_visualizer->setDialDisplays(vDial, tDial);

    connect(m_visualizer, &DataVisualizer::log, this, &DataManager::log);

    // 创建CSV写入器
    m_writer = new CSVWriter(this);
    m_writer->setDataSource(m_buffer);

    connect(m_writer, &CSVWriter::log, this, &DataManager::log);
    connect(m_writer, &CSVWriter::fileRotated,
            this, [this](const QString& file) {
                emit log(Logger::Type::File, QString("文件切换: %1").arg(file));
                emit fileRotated();
            });


    emit log(Logger::Type::System, "DataManager 初始化完成");
}

void DataManager::startRecording(bool enableStorage) {
    if (m_isRecording) return;

    m_isRecording = true;

    if (m_visualizer) {
        m_visualizer->start();
    }

    m_buffer->setEnableStorage(enableStorage);
    // 根据参数决定是否启动 Writer
    if (m_writer && enableStorage) {
        m_writer->start();
    }

    emit recordingStarted();
}

void DataManager::stopRecording() {
    if (!m_isRecording) return;

    m_isRecording = false;

    if (m_visualizer) m_visualizer->stop();
    if (m_writer) m_writer->stop();

    emit log(Logger::Type::System, "数据记录已停止");
}

void DataManager::pauseDisplay() {
    if (!m_visualizer || m_isPaused) return;

    m_isPaused = true;
    m_visualizer->stop();
    emit log(Logger::Type::Info, "显示已暂停");
}

void DataManager::resumeDisplay() {
    if (!m_visualizer || !m_isPaused) return;

    m_isPaused = false;
    if (m_isRecording) {
        m_visualizer->start();
    }
    emit log(Logger::Type::Info, "显示已恢复");
}

void DataManager::onSampleReceived(const Sample& sample) {
    if (!m_isRecording || !m_buffer) return;

    m_buffer->write(sample);
    emit bufferStatusChanged(m_buffer->size(), m_buffer->capacity());
}

void DataManager::onSamplesReceived(const QVector<Sample>& samples) {
    if (!m_isRecording || !m_buffer || samples.isEmpty()) return;

    m_buffer->writeBatch(samples);
    emit bufferStatusChanged(m_buffer->size(), m_buffer->capacity());
}

void DataManager::onSampleReceivedDirect(const Sample& sample) {
    // 文件回放模式：直接构造数据给 Visualizer，不经过 Buffer
    if (!m_isRecording || !m_visualizer) return;

    // 手动构造批次数据（或积累后批量发送）
    static QVector<Sample> batch;
    batch.append(sample);

    if (batch.size() >= 10) {  // 批量大小
        // 直接调用 visualizer 的更新方法
        // 需要给 DataVisualizer 添加一个直接接收数据的方法
        batch.clear();
    }
}

void DataManager::clearChart(){
    m_visualizer->clear();
}
