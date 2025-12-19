#include "frmcomtool.h"
#include "qthelper.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QtHelper::initMain();
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/main.ico"));

    // 注册 Logger::Type 类型
    qRegisterMetaType<Logger::Type>("Logger::Type");

    // 设置编码以及加载中文翻译文件
    QtHelper::initAll();
    // 读取配置文件
    AppConfig::ConfigFile = QString("%1/%2.ini").arg(QtHelper::appPath()).arg(QtHelper::appName());
    AppConfig::readConfig();

    AppData::Intervals << "1" << "10" << "20" << "50" << "100" << "200" << "300" << "500" << "1000" << "1500" << "2000" << "3000" << "5000" << "10000";
    AppData::readSendData();
    AppData::readDeviceData();

    frmComTool w;

    w.setWindowTitle("UESTC信软学院数信方向63组综设demo V2.0");
    w.resize(900, 650);
    QtHelper::setFormInCenter(&w);
    w.show();

    return a.exec();
}
