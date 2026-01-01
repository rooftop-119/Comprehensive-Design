#include "appconfig.h"
#include "qthelper.h"

QString AppConfig::ConfigFile = "config.ini";

QString AppConfig::PortName = "COM4";
int AppConfig::BaudRate = 9600;
int AppConfig::DataBit = 8;
QString AppConfig::Parity = QString::fromUtf8("无");
double AppConfig::StopBit = 1;

bool AppConfig::AutoSave = true;
int AppConfig::SaveInterval = 500;

// 缓冲区配置
int AppConfig::buffer_capacity = 200;

// 定时器配置
int AppConfig::painter_interval = 50;
int AppConfig::reader_interval = 100;
int AppConfig::linesPerTick = 3;

// 命令超时配置
int AppConfig::COMMAND_TIMEOUT_MS = 3000;

// 显示配置
bool AppConfig::antialiasing = true;
double AppConfig::plot_time_window = 12;
int AppConfig::plot_max_points = 20000;
int AppConfig::plot_batch_size = 6;
bool AppConfig::autoRescaleY = true;
bool AppConfig::autoRescaleY_onlyEnlarge = false;
double AppConfig::voltMin = 0.0;
double AppConfig::voltMax = 2.0;
double AppConfig::tempMin = 0.0;
double AppConfig::tempMax = 40.0;
double AppConfig::voltLowThreshold = 0.0;
double AppConfig::voltHighThreshold = 2.0;
double AppConfig::tempLowThreshold = 20.0;
double AppConfig::tempHighThreshold = 30.0;

// 日志配置
int AppConfig::logger_max_lines = 50;
bool AppConfig::DataShow = false;

void AppConfig::readConfig()
{
    QSettings set(ConfigFile, QSettings::IniFormat);

    // 串口配置
    set.beginGroup("ComConfig");
    PortName = set.value("PortName", PortName).toString();
    BaudRate = set.value("BaudRate", BaudRate).toInt();
    DataBit  = set.value("DataBit", DataBit).toInt();
    Parity   = set.value("Parity", Parity).toString();
    StopBit  = set.value("StopBit", StopBit).toDouble();
    set.endGroup();

    // 文件与采集
    set.beginGroup("FileConfig");
    AutoSave        = set.value("AutoSave", AutoSave).toBool();
    SaveInterval    = set.value("SaveInterval", SaveInterval).toInt();
    reader_interval = set.value("reader_interval", reader_interval).toInt();
    linesPerTick    = set.value("linesPerTick", linesPerTick).toInt();
    set.endGroup();

    // 缓冲区
    set.beginGroup("BufferConfig");
    buffer_capacity = set.value("buffer_capacity", buffer_capacity).toInt();
    DataShow        = set.value("DataShow", DataShow).toBool();
    set.endGroup();

    // 绘图定时器
    set.beginGroup("PainterConfig");
    painter_interval   = set.value("painter_interval", painter_interval).toInt();
    plot_time_window   = set.value("plot_time_window", plot_time_window).toDouble();
    plot_max_points    = set.value("plot_max_points", plot_max_points).toInt();
    plot_batch_size    = set.value("plot_batch_size", plot_batch_size).toInt();
    antialiasing       = set.value("antialiasing", antialiasing).toBool();
    autoRescaleY       = set.value("autoRescaleY", autoRescaleY).toBool();
    autoRescaleY_onlyEnlarge = set.value("autoRescaleY_onlyEnlarge", autoRescaleY_onlyEnlarge).toBool();
    set.endGroup();

    // 阈值与范围
    set.beginGroup("ThresholdConfig");
    voltMin = set.value("voltMin", voltMin).toDouble();
    voltMax = set.value("voltMax", voltMax).toDouble();
    tempMin = set.value("tempMin", tempMin).toDouble();
    tempMax = set.value("tempMax", tempMax).toDouble();

    voltLowThreshold     = set.value("voltLowThreshold", voltLowThreshold).toDouble();
    voltHighThreshold    = set.value("voltHighThreshold", voltHighThreshold).toDouble();
    tempLowThreshold     = set.value("tempLowThreshold", tempLowThreshold).toDouble();
    tempHighThreshold    = set.value("tempHighThreshold", tempHighThreshold).toDouble();
    set.endGroup();

    // 其他配置
    set.beginGroup("OtherConfig");
    COMMAND_TIMEOUT_MS = set.value("COMMAND_TIMEOUT_MS", COMMAND_TIMEOUT_MS).toInt();
    logger_max_lines   = set.value("logger_max_lines", logger_max_lines).toInt();
    set.endGroup();

    // 如果配置文件缺失或损坏，生成默认配置（但不覆盖已有项）
    // QtHelper::checkIniFile 假设返回 false 表示文件不存在或格式错误
    if (!QtHelper::checkIniFile(ConfigFile)) {
        writeConfig();  // 生成完整的默认配置文件
    }
}

void AppConfig::writeConfig()
{
    QSettings set(ConfigFile, QSettings::IniFormat);

    set.beginGroup("ComConfig");
    set.setValue("PortName", PortName);
    set.setValue("BaudRate", BaudRate);
    set.setValue("DataBit", DataBit);
    set.setValue("Parity", Parity);
    set.setValue("StopBit", StopBit);
    set.endGroup();

    set.beginGroup("FileConfig");
    set.setValue("AutoSave", AutoSave);
    set.setValue("SaveInterval", SaveInterval);
    set.setValue("reader_interval", reader_interval);
    set.setValue("linesPerTick", linesPerTick);
    set.endGroup();

    set.beginGroup("BufferConfig");
    set.setValue("buffer_capacity", buffer_capacity);
    set.setValue("DataShow", DataShow);
    set.endGroup();

    set.beginGroup("PainterConfig");
    set.setValue("painter_interval", painter_interval);
    set.setValue("plot_time_window", plot_time_window);
    set.setValue("plot_max_points", plot_max_points);
    set.setValue("plot_batch_size", plot_batch_size);
    set.setValue("antialiasing", antialiasing);
    set.setValue("autoRescaleY", autoRescaleY);
    set.setValue("autoRescaleY_onlyEnlarge", autoRescaleY_onlyEnlarge);
    set.endGroup();

    set.beginGroup("ThresholdConfig");
    set.setValue("voltMin", voltMin);
    set.setValue("voltMax", voltMax);
    set.setValue("tempMin", tempMin);
    set.setValue("tempMax", tempMax);
    set.setValue("voltLowThreshold", voltLowThreshold);
    set.setValue("voltHighThreshold", voltHighThreshold);
    set.setValue("tempLowThreshold", tempLowThreshold);
    set.setValue("tempHighThreshold", tempHighThreshold);
    set.endGroup();

    set.beginGroup("OtherConfig");
    set.setValue("COMMAND_TIMEOUT_MS", COMMAND_TIMEOUT_MS);
    set.setValue("logger_max_lines", logger_max_lines);
    set.endGroup();

    set.sync();  // 强制写入磁盘
}
