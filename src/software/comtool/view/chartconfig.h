#ifndef CHARTCONFIG_H
#define CHARTCONFIG_H

#include <QColor>

struct ChartConfig {
    // 更新配置
    int updateInterval = 50;        // ms
    int batchSize = 10;             // 每次从buffer读取的数量

    // 显示配置
    double timeWindow = 10.0;       // 秒
    int maxPoints = 20000;          // 最大点数
    bool autoRescaleY = true;      // 自动缩放Y轴

    // 电压配置
    double voltMin = 0.0;
    double voltMax = 65.535;
    QColor voltColor = QColor(76, 175, 80);
    QColor voltFill = QColor(76, 175, 80, 40);

    // 温度配置
    double tempMin = -327.68;
    double tempMax = 327.67;
    QColor tempColor = QColor(33, 150, 243);
    QColor tempFill = QColor(33, 150, 243, 30);

    // LCD报警阈值
    double voltHighThreshold = 64.0;
    double voltLowThreshold = 0.0;
    double tempHighThreshold = 333.0;
    double tempWarnThreshold = -333.0;
};

#endif
