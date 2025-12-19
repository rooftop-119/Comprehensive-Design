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
double AppConfig::plot_time_window = 12;
int AppConfig::plot_max_points = 20000;
int AppConfig::plot_batch_size = 6;

// 日志配置
int AppConfig::logger_max_lines = 50;

void AppConfig::readConfig()
{
    QSettings set(AppConfig::ConfigFile, QSettings::IniFormat);

    set.beginGroup("ComConfig");
    AppConfig::PortName = set.value("PortName", AppConfig::PortName).toString();
    AppConfig::BaudRate = set.value("BaudRate", AppConfig::BaudRate).toInt();
    AppConfig::DataBit = set.value("DataBit", AppConfig::DataBit).toInt();
    AppConfig::Parity = set.value("Parity", AppConfig::Parity).toString();
    AppConfig::StopBit = set.value("StopBit", AppConfig::StopBit).toInt();
    set.endGroup();

    set.beginGroup("FileConfig");
    AppConfig::AutoSave = set.value("AutoSave", AppConfig::AutoSave).toBool();
    AppConfig::SaveInterval = set.value("SaveInterval", AppConfig::SaveInterval).toInt();
    AppConfig::reader_interval = set.value("reader_interval", AppConfig::reader_interval).toInt();
    AppConfig::linesPerTick = set.value("linesPerTick", AppConfig::linesPerTick).toInt();
    set.endGroup();

    set.beginGroup("BufferConfig");
    AppConfig::buffer_capacity = set.value("buffer_capacity", AppConfig::buffer_capacity).toInt();
    set.endGroup();

    set.beginGroup("PainterConfig");
    AppConfig::painter_interval = set.value("painter_interval", AppConfig::painter_interval).toInt();
    AppConfig::plot_time_window = set.value("plot_time_window", AppConfig::plot_time_window).toInt();
    AppConfig::plot_max_points = set.value("plot_max_points", AppConfig::plot_max_points).toInt();
    AppConfig::plot_batch_size = set.value("plot_batch_size", AppConfig::plot_batch_size).toInt();
    set.endGroup();

    set.beginGroup("OtherConfig");
    AppConfig::COMMAND_TIMEOUT_MS = set.value("COMMAND_TIMEOUT_MS", AppConfig::COMMAND_TIMEOUT_MS).toInt();
    AppConfig::logger_max_lines = set.value("logger_max_lines", AppConfig::logger_max_lines).toInt();
    set.endGroup();

    //配置文件不存在或者不全则重新生成
    if (!QtHelper::checkIniFile(AppConfig::ConfigFile)) {
        writeConfig();
        return;
    }
}

void AppConfig::writeConfig()
{    
    QSettings set(AppConfig::ConfigFile, QSettings::IniFormat);

    set.beginGroup("ComConfig");
    set.setValue("PortName", AppConfig::PortName);
    set.setValue("BaudRate", AppConfig::BaudRate);
    set.setValue("DataBit", AppConfig::DataBit);
    set.setValue("Parity", AppConfig::Parity);
    set.setValue("StopBit", AppConfig::StopBit);
    set.endGroup();

    set.beginGroup("FileConfig");
    set.setValue("AutoSave", AppConfig::AutoSave);
    set.setValue("SaveInterval", AppConfig::SaveInterval);
    set.setValue("reader_interval", AppConfig::reader_interval);
    set.setValue("linesPerTick", AppConfig::linesPerTick);
    set.endGroup();

    set.beginGroup("BufferConfig");
    set.setValue("buffer_capacity", AppConfig::buffer_capacity);
    set.endGroup();

    set.beginGroup("PainterConfig");
    set.setValue("painter_interval", AppConfig::painter_interval);
    set.setValue("plot_time_window", AppConfig::plot_time_window);
    set.setValue("plot_max_points", AppConfig::plot_max_points);
    set.setValue("plot_batch_size", AppConfig::plot_batch_size);
    set.endGroup();

    set.beginGroup("OtherConfig");
    set.setValue("COMMAND_TIMEOUT_MS", AppConfig::COMMAND_TIMEOUT_MS);
    set.setValue("logger_max_lines", AppConfig::logger_max_lines);
    set.endGroup();
}
