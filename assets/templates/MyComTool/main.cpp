#include "frmcomtool.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序信息
    a.setApplicationName("串口曲线工具");
    a.setApplicationVersion("1.0");

    frmComTool w;
    w.show();

    return a.exec();
}
