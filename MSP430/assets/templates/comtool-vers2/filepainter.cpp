#include "filepainter.h"

FilePainter::FilePainter(QCustomPlot* board, FileBuffer* buf,QObject* parent)
    : QObject(parent), m_plot(board),fileBuffer(buf)
{
    timer = new QTimer(this);
    timer->setInterval(interval);

    connect(timer, &QTimer::timeout, this, &FilePainter::onRefreshTimeout);
}

void FilePainter::start()
{
    emit log(Logger::Type::System,"FilePainter started");
    timer->start();
}

void FilePainter::stop()
{
    timer->stop();
    emit log(Logger::Type::Info,"已完成绘图");
}

void FilePainter::onRefreshTimeout()
{
    if (!fileBuffer) return;

    auto batch = fileBuffer->readForPlot(m_batchSize);
    if (batch.isEmpty())
        return;

    double lastTime = batch.last().first;

    appendBatchToPlot(batch);

    // 删除旧数据
    for (int i = 0; i < m_plot->graphCount(); ++i)
        m_plot->graph(i)->removeDataBefore(lastTime - m_timeWindowSec);

    // 移动窗口
    m_plot->xAxis->setRange(lastTime - m_timeWindowSec, lastTime);

    // 高性能重绘
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void FilePainter::appendBatchToPlot(const QVector<QPair<double,double>> &batch)
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
