#include "frmcomtool.h"
#include "qthelper.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QtHelper::initMain();
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/main.ico"));

    // 1. 强制使用 Fusion 风格
    a.setStyle(QStyleFactory::create("Fusion"));

    // 2. 强制自定义调色板（彻底抵抗系统深色模式和高对比度）
    QPalette palette = a.palette();
    palette.setColor(QPalette::Window, QColor(240, 240, 240));         // 主窗口背景
    palette.setColor(QPalette::WindowText, Qt::black);
    palette.setColor(QPalette::Base, Qt::white);                      // 输入框、QComboBox 背景
    palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::black);
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Button, QColor(240, 240, 240));
    palette.setColor(QPalette::ButtonText, Qt::black);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(0, 122, 204));

    // 高亮（选中）颜色
    palette.setColor(QPalette::Highlight, QColor(51, 153, 255));  // #3399ff
    palette.setColor(QPalette::HighlightedText, Qt::white);           // 高亮文字（下拉列表选中项用白色）

    // 禁用状态（防止灰太淡）
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));

    a.setPalette(palette);

    // 3. 全局 QSS：专门强化 QComboBox（解决所有历史问题）
    a.setStyleSheet(
        "QComboBox {"
        "   background-color: white;"
        "   color: black;"                      /* 强制当前文字黑色 */
        "   border: 1px solid #a0a0a0;"
        "   border-radius: 4px;"
        "   padding: 6px 24px 6px 8px;"         /* 右边留空间给箭头 */
        "   min-height: 20px;"
        "}"
        "QComboBox:hover {"
        "   border: 1px solid #3399ff;"
        "}"
        "QComboBox::drop-down {"
        "   subcontrol-origin: padding;"
        "   subcontrol-position: right center;"
        "   width: 24px;"
        "   border-left: 1px solid #a0a0a0;"
        "}"
        "QComboBox::down-arrow {"
        "   width: 12px;"
        "   height: 12px;"
        "}"
        /* 关键：下拉菜单打开时，当前显示项文字不会变白 */
        "QComboBox:!editable:on {"
        "   color: black;"
        "   background-color: white;"
        "}"
        /* 下拉列表样式 */
        "QComboBox QAbstractItemView {"
        "   background-color: white;"
        "   color: black;"                                      /* 未选中项文字黑色 */
        "   selection-background-color: #3399ff;"               /* 选中背景蓝色 */
        "   selection-color: white;"                           /* 选中项文字白色（对比强烈） */
        "   border: 1px solid #a0a0a0;"
        "   outline: none;"
        "}"
        "QComboBox:disabled {"
        "   background-color: #f0f0f0;"
        "   color: #808080;"
        "}"
        );

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
    w.setWindowTitle("UESTC信软学院数信方向63组综设demo V2.4.1");
    w.resize(900, 650);
    QtHelper::setFormInCenter(&w);
    w.show();

    return a.exec();
}
