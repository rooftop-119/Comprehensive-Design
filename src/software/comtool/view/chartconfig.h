#ifndef CHARTCONFIG_H
#define CHARTCONFIG_H
#include "appconfig.h"

#include <QColor>

struct ChartConfig {

    bool antialiasing = AppConfig::antialiasing;

    // 更新配置
    int updateInterval = AppConfig::painter_interval;        // ms
    int batchSize = AppConfig::plot_batch_size;             // 每次从buffer读取的数量

    // 显示配置
    double timeWindow = AppConfig::plot_time_window;       // 秒
    int maxPoints = AppConfig::plot_max_points;          // 最大点数
    bool autoRescaleY_onlyEnlarge = AppConfig::autoRescaleY_onlyEnlarge;      // 自动缩放Y轴
    bool autoRescaleY = AppConfig::autoRescaleY;

    // 电压配置
    double voltMin = AppConfig::voltMin;
    double voltMax = AppConfig::voltMax;
    QColor voltColor = QColor(76, 175, 80);
    QColor voltFill = QColor(76, 175, 80, 40);

    // 温度配置
    double tempMin = AppConfig::tempMin;
    double tempMax = AppConfig::tempMax;
    QColor tempColor = QColor(33, 150, 243);
    QColor tempFill = QColor(33, 150, 243, 30);

    // LCD报警阈值
    double voltHighThreshold = AppConfig::voltHighThreshold;
    double voltLowThreshold = AppConfig::voltLowThreshold;
    double tempHighThreshold = AppConfig::tempHighThreshold;
    double tempWarnThreshold = AppConfig::tempLowThreshold;
};

#endif
