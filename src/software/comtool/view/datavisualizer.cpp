#include "datavisualizer.h"

DataVisualizer::DataVisualizer(QCustomPlot* plot, QObject* parent)
    : QObject(parent), m_plot(plot)
{
    setupChart();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DataVisualizer::onUpdateTimeout);
}

DataVisualizer::~DataVisualizer() {
    stop();
}

void DataVisualizer::setDataSource(DataBuffer* buffer) {
    m_dataSource = buffer;
}

void DataVisualizer::setLCDDisplays(QLCDNumber* voltLcd, QLCDNumber* tempLcd) {
    m_voltLcd = voltLcd;
    m_tempLcd = tempLcd;
}

void DataVisualizer::setDialDisplays(QDial* voltDial, QDial* tempDial) {
    m_voltDial = voltDial;
    m_tempDial = tempDial;
}

void DataVisualizer::setConfig(const ChartConfig& config) {
    m_config = config;

    m_timer->setInterval(m_config.updateInterval);

    // 更新图表范围
    m_plot->yAxis->setRange(m_config.voltMin, m_config.voltMax);
    m_plot->yAxis2->setRange(m_config.tempMin, m_config.tempMax);

    // 更新颜色（修改这里）
    if (m_voltGraph) {
        m_voltGraph->setPen(QPen(m_config.voltColor, 2));
    }
    if (m_tempGraph) {
        m_tempGraph->setPen(QPen(m_config.tempColor, 2));
    }

    m_plot->replot();
}

void DataVisualizer::start() {
    if (!m_dataSource) {
        emit log(Logger::Type::System, "DataVisualizer: 未设置数据源");
        return;
    }

    m_timer->start();
    emit log(Logger::Type::Info, "可视化已启动");
}

void DataVisualizer::stop() {
    m_timer->stop();
    emit log(Logger::Type::Info, "可视化已停止");
}

void DataVisualizer::clear() {
    if (m_voltGraph) {
        m_voltGraph->data()->clear();
    }
    if (m_tempGraph) {
        m_tempGraph->data()->clear();
    }
    m_plot->replot();
    emit log(Logger::Type::Info, "图表已清空");
}

void DataVisualizer::setupChart() {
    if (!m_plot) return;

    // 清除现有图形
    m_plot->clearGraphs();

    m_plot->setNotAntialiasedElements(QCP::aeAll);
    m_plot->setAntialiasedElements(QCP::aeNone);

    // 左侧电压曲线
    m_voltGraph = m_plot->addGraph(m_plot->xAxis, m_plot->yAxis);
    m_voltGraph->setPen(QPen(m_config.voltColor, 1));
    m_voltGraph->setBrush(QBrush(m_config.voltFill));
    m_voltGraph->setLineStyle(QCPGraph::lsLine);

    m_plot->yAxis->setLabel("电压 (V)");
    m_plot->yAxis->setRange(m_config.voltMin, m_config.voltMax);

    // 右侧温度曲线
    m_plot->yAxis2->setVisible(true);
    m_plot->yAxis2->setLabel("温度 (°C)");
    m_plot->yAxis2->setRange(m_config.tempMin, m_config.tempMax);

    m_tempGraph = m_plot->addGraph(m_plot->xAxis, m_plot->yAxis2);
    m_tempGraph->setPen(QPen(m_config.tempColor, 1));
    m_tempGraph->setBrush(QBrush(m_config.tempFill));
    m_tempGraph->setLineStyle(QCPGraph::lsLine);

    // 创建0基线图（温度曲线填充到0轴）
    // QCPGraph *zeroLineGraph = m_plot->addGraph(m_plot->xAxis, m_plot->yAxis2);
    // zeroLineGraph->setPen(Qt::NoPen);
    // zeroLineGraph->setBrush(Qt::NoBrush);
    // zeroLineGraph->removeFromLegend();

    // double baseValue = 0;
    // zeroLineGraph->addData(0, baseValue);  // 初始点
    // zeroLineGraph->addData(1000000000, baseValue);

    // 重要：使用不同的Y轴，避免targetGraph is this graph itself错误
    // m_tempGraph->setChannelFillGraph(zeroLineGraph);

    // 时间轴
    QSharedPointer<QCPAxisTickerDateTime> ticker(new QCPAxisTickerDateTime);
    ticker->setDateTimeFormat("hh:mm:ss");
    ticker->setDateTimeSpec(Qt::LocalTime);
    m_plot->xAxis->setTicker(ticker);

    double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    m_plot->xAxis->setRange(now - m_config.timeWindow, now);

    m_voltGraph->data()->setAutoSqueeze(true);
    m_tempGraph->data()->setAutoSqueeze(true);

    m_plot->legend->setVisible(true);
    m_voltGraph->setName("Voltage");
    m_tempGraph->setName("Temperature");

    // 交互
    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    m_plot->replot();
}

void DataVisualizer::onUpdateTimeout() {
    if (!m_dataSource || m_dataSource->isEmpty()) return;

    // 从缓冲区读取数据
    QVector<Sample> samples = m_dataSource->readForVisualize(m_config.batchSize);
    if (samples.isEmpty()) return;

    updateChart(samples);

    // 更新LCD和Dial（使用最新数据）
    if (!samples.isEmpty()) {
        updateLCDs(samples.last());
        updateDials(samples.last());
    }
}

void DataVisualizer::updateWithData(const QVector<Sample>& samples) {
    if (samples.isEmpty()) return;

    updateChart(samples);

    if (!samples.isEmpty()) {
        updateLCDs(samples.last());
        updateDials(samples.last());
    }
}

void DataVisualizer::updateChart(const QVector<Sample>& samples) {
    if (samples.isEmpty()) return;

    double latestTime = samples.last().timestamp;

    // 准备数据向量
    QVector<double> voltKeys, voltValues;
    QVector<double> tempKeys, tempValues;

    for (const auto& sample : samples) {        
        if (sample.hasVoltage) {
            voltKeys.append(sample.timestamp);
            voltValues.append(sample.voltage);
        }

        if (sample.hasTemperature) {
            tempKeys.append(sample.timestamp);
            tempValues.append(sample.temperature);
        }
    }

    // 添加数据到图表（修改这里）
    if (!voltKeys.isEmpty() && m_voltGraph) {
        m_voltGraph->addData(voltKeys, voltValues);
    }

    if (!tempKeys.isEmpty() && m_tempGraph) {
        m_tempGraph->addData(tempKeys, tempValues);
    }

    // 删除旧数据
    removeOldData(latestTime);

    // 更新X轴范围
    m_plot->xAxis->setRange(latestTime - m_config.timeWindow, latestTime);

    // 可选：自动缩放Y轴（修改这里）
    if (m_config.autoRescaleY) {
        if (m_voltGraph) m_voltGraph->rescaleValueAxis(false, true);
        if (m_tempGraph) m_tempGraph->rescaleValueAxis(false, true);
    }

    // 高性能重绘
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void DataVisualizer::updateLCDs(const Sample& latest) {
    if (m_voltLcd && latest.hasVoltage) {
        m_voltLcd->display(QString::number(latest.voltage, 'f', 3));

        // 根据阈值改变颜色
        QString style;
        if (latest.voltage > m_config.voltHighThreshold) {
            style = "color: #ff4444; background-color: #0d1117; border: 2px solid #ff4444;";
        } else if (latest.voltage < m_config.voltLowThreshold) {
            style = "color: #ffaa00; background-color: #0d1117; border: 2px solid #ffaa00;";
        } else {
            style = "color: #66bb6a; background-color: #0d1117; border: 2px solid #66bb6a;";
        }
        m_voltLcd->setStyleSheet("QLCDNumber {" + style + " border-radius: 10px; padding: 5px;}");
    }

    if (m_tempLcd && latest.hasTemperature) {
        m_tempLcd->display(QString::number(latest.temperature, 'f', 2));

        QString style;
        if (latest.temperature > m_config.tempHighThreshold) {
            style = "color: #ff4444; background-color: #0d1117; border: 2px solid #ff4444;";
        } else if (latest.temperature > m_config.tempWarnThreshold) {
            style = "color: #ffaa00; background-color: #0d1117; border: 2px solid #ffaa00;";
        } else {
            style = "color: #29b6f6; background-color: #0d1117; border: 2px solid #29b6f6;";
        }
        m_tempLcd->setStyleSheet("QLCDNumber {" + style + " border-radius: 10px; padding: 5px;}");
    }
}

void DataVisualizer::updateDials(const Sample& latest) {
    if (m_voltDial && latest.hasVoltage) {
        m_voltDial->setValue(static_cast<int>(latest.voltage * 1000));
    }

    if (m_tempDial && latest.hasTemperature) {
        m_tempDial->setValue(static_cast<int>(latest.temperature));
    }
}

void DataVisualizer::removeOldData(double currentTime) {
    double cutoffTime = currentTime - m_config.timeWindow;

    // 删除过期数据（修改这里）
    if (m_voltGraph) {
        m_voltGraph->data()->removeBefore(cutoffTime);
    }
    if (m_tempGraph) {
        m_tempGraph->data()->removeBefore(cutoffTime);
    }

    // 限制最大点数（修改这里）
    if (m_voltGraph && m_voltGraph->dataCount() > m_config.maxPoints) {
        m_voltGraph->data()->remove(0, m_voltGraph->dataCount() - m_config.maxPoints);
    }

    if (m_tempGraph && m_tempGraph->dataCount() > m_config.maxPoints) {
        m_tempGraph->data()->remove(0, m_tempGraph->dataCount() - m_config.maxPoints);
    }
}
