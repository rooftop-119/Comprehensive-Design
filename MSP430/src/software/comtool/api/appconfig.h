#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include "global.h"

class AppConfig
{
public:
    static QString ConfigFile;          //配置文件路径

    static QString PortName;            //串口号
    static int BaudRate;                //波特率
    static int DataBit;                 //数据位
    static QString Parity;              //校验位
    static double StopBit;              //停止位

    static bool AutoSave;               //自动保存
    static int SaveInterval;            // 文件写入间隔，默认 5000ms

    // 缓冲区配置
    static int buffer_capacity;      // 默认 200

    // 定时器配置
    static int painter_interval;     // 绘图刷新间隔，默认 50ms
    static int reader_interval;      // 文件读取间隔，默认 100ms
    static int linesPerTick;

    // 命令超时配置
    static int COMMAND_TIMEOUT_MS;      // 命令超时时间，默认 3000ms

    // 显示配置
    static bool antialiasing;
    static double plot_time_window;  // 绘图时间窗口，默认 10s
    static int plot_max_points;      // 最大绘图点数，默认 20000
    static int plot_batch_size;      // 每次绘图批量，默认 6
    static bool autoRescaleY;
    static bool autoRescaleY_onlyEnlarge;
    static double voltMin;
    static double voltMax;
    static double tempMin;
    static double tempMax;
    static double voltHighThreshold;
    static double voltLowThreshold;
    static double tempHighThreshold;
    static double tempLowThreshold;

    // 日志配置
    static int logger_max_lines;     // 日志最大行数，默认 100
    static bool DataShow;

    //读写配置参数
    static void readConfig();           //读取配置参数
    static void writeConfig();          //写入配置参数
};

#endif // APPCONFIG_H
