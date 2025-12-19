#include "serialpainter.h"

SerialPainter::SerialPainter(QCustomPlot* board, DataBuffer* buffer, QObject* parent)
    : QObject(parent), m_plot(board), serialBuffer(buffer)
{
    timer = new QTimer(this);
    timer->setInterval(interval);
    connect(timer, &QTimer::timeout, this, &SerialPainter::onRefreshTimeout);
}

void SerialPainter::start() {
    timer->start();
}

void SerialPainter::stop() {
    timer->stop();
}

void SerialPainter::onRefreshTimeout()
{
    auto batch = serialBuffer->readForPlot(m_batchSize);
    if (batch.isEmpty())
        return;
    
    double lastTime = batch.last().first;

    appendBatchToPlot(batch);

    // 删除过期数据（所有曲线）
    for (int i = 0; i < m_plot->graphCount(); ++i)
        m_plot->graph(i)->removeDataBefore(lastTime - m_timeWindowSec);

    // X轴移动
    m_plot->xAxis->setRange(lastTime - m_timeWindowSec, lastTime);

    // 自适应 Y 轴，必要时开启
    if (false) {
        m_plot->graph(0)->rescaleValueAxis(true, true);  // Voltage
        m_plot->graph(1)->rescaleValueAxis(true, true);  // Temperature
    }

    // 高性能重绘
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void SerialPainter::appendBatchToPlot(const QVector<QPair<double,double>> &batch)
{
    QVector<double> keysV, keysT;
    QVector<double> valsV, valsT;

    keysV.reserve(batch.size());
    keysT.reserve(batch.size());
    valsV.reserve(batch.size());
    valsT.reserve(batch.size());

    for (const auto &p : batch) {
        double t = p.first;
        double v = p.second;

        if (v < 6.0) {      // Voltage channel
            keysV.append(t);
            valsV.append(v);
        } else {            // Temperature channel
            keysT.append(t);
            valsT.append(v);
        }
    }

    if (!keysV.isEmpty())
        m_plot->graph(0)->addData(keysV, valsV);

    if (!keysT.isEmpty())
        m_plot->graph(1)->addData(keysT, valsT);
}
