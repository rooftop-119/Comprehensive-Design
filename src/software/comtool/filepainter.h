#ifndef FILEPAINTER_H
#define FILEPAINTER_H

#include <QObject>
#include "qcustomplot.h"
#include "logger.h"
#include "filebuffer.h"

class FilePainter : public QObject
{
    Q_OBJECT
public:
    explicit FilePainter(QCustomPlot* board, FileBuffer* buf,QObject* parent = nullptr);

    void start();
    void stop();
signals:
    void log(Logger::Type type, const QString &msg, bool clear=false);

private:
    void onRefreshTimeout();
    void appendBatchToPlot(const QVector<QPair<double,double>> &batch);

private:
    QCustomPlot* m_plot = nullptr;
    FileBuffer* fileBuffer = nullptr;
    QTimer* timer = nullptr;
    int interval = 75;
    double m_timeWindowSec = 10.0;
    int m_maxPlotPoints = 20000;
    int m_batchSize = 6;
};

#endif // FILEPAINTER_H
