#ifndef DATAVISUALIZER_H
#define DATAVISUALIZER_H

#include <QObject>
#include <QTimer>
#include "qcustomplot.h"
#include "core/databuffer.h"
#include <QLCDNumber>
#include <QDial>
#include "chartconfig.h"

class DataVisualizer : public QObject {
    Q_OBJECT
public:
    explicit DataVisualizer(QCustomPlot* plot, QObject* parent = nullptr);
    ~DataVisualizer();

    // 配置接口
    void setDataSource(DataBuffer* buffer);
    void setLCDDisplays(QLCDNumber* voltLcd, QLCDNumber* tempLcd);
    void setDialDisplays(QDial* voltDial, QDial* tempDial);
    void setConfig(const ChartConfig& config);

    // 控制接口
    void start();
    void stop();
    void clear();

    // 查询接口
    bool isRunning() const { return m_timer && m_timer->isActive(); }

    void updateWithData(const QVector<Sample>& samples);

    void setAutoRescaleY(bool onlyEnlarge,bool includeErrorBars);
    void updateConf();
    void updateLCDs(const Sample& latest);

signals:
    void log(Logger::Type type, const QString& msg, bool clear = false);

private slots:
    void onUpdateTimeout();

private:
    void setupChart();
    void updateChart(const QVector<Sample>& samples);
    void updateDials(const Sample& latest);
    void removeOldData(double currentTime);

    QCustomPlot* m_plot = nullptr;
    DataBuffer* m_dataSource = nullptr;
    QTimer* m_timer = nullptr;

    // LCD 和 Dial 显示
    QLCDNumber* m_voltLcd = nullptr;
    QLCDNumber* m_tempLcd = nullptr;
    QDial* m_voltDial = nullptr;
    QDial* m_tempDial = nullptr;

    ChartConfig m_config;

    QCPGraph* m_voltGraph = nullptr;
    QCPGraph* m_tempGraph = nullptr;
};

#endif
