#ifndef SERIALPAINTER_H
#define SERIALPAINTER_H

#include <QObject>
#include "qcustomplot.h"
#include "logger.h"
#include "databuffer.h"

class SerialPainter : public QObject
{
    Q_OBJECT
public:
    explicit SerialPainter(QCustomPlot* board,DataBuffer* buffer, QObject* parent = nullptr);

    void start();
    void stop();

signals:
    void log(Logger::Type type, const QString &msg, bool clear=false);

private:
    void onRefreshTimeout();
    void appendBatchToPlot(const QVector<QPair<double,double>> &batch);

private:
    QTimer* timer = nullptr;
    QCustomPlot* m_plot = nullptr;
    DataBuffer* serialBuffer = nullptr;
    int interval = 50;
    double m_timeWindowSec = 10.0; // x轴显示窗口长度（秒）
    int m_maxPlotPoints = 20000; // 图上最大点数
    int m_batchSize = 6; // 每次从 buffer 取多少点
};

#endif // PAINTER_H
