#ifndef FRMCOMTOOL_H
#define FRMCOMTOOL_H

#include <QWidget>
#include "qtcpsocket.h"
#include "head.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#include "DataBuffer.h"
#include <QFile>
#include "filewriter.h"
#include "filereader.h"
#include "filebuffer.h"
#include "logger.h"
#include "filepainter.h"
#include "serialpainter.h"
class QCustomPlot;
class QTimer;
class SerialWorker;
class FileWriter;
class FileReader;
class FileBuffer;

namespace Ui
{
class frmComTool;
}

class frmComTool : public QWidget
{
    Q_OBJECT

public:
    explicit frmComTool(QWidget *parent = 0);
    ~frmComTool();

private:
    Ui::frmComTool *ui;

    QCustomPlot *m_plot = nullptr;
    Logger* logger = nullptr;

    SerialWorker *worker = nullptr;
    DataBuffer *m_buffer = nullptr;
    FileBuffer *filebuffer = nullptr;
    FileWriter *fileWriter = nullptr;
    FileReader *fileReader = nullptr;
    FilePainter *filePainter = nullptr;
    SerialPainter *serialPainter = nullptr;

    QThread* serialThread;
    QThread* fileThread;

    bool comOk;                 //串口是否打开
    int sleepTime;              //接收延时时间
    int sendCount;              //发送数据计数
    int receiveCount;           //接收数据计数
    bool isShow;                //是否显示数据

private:
    double m_timeWindowSec = 10.0;
    QStringList getRecentCsvFiles(const QString &dirPath, int count);
    void init();
    void setUpPlot();

signals:
    void openPortRequest(const QString &portName, int baudRate, int dataBits,
                         const QString &parity, double stopBits);
    void closePortRequest();
    void writeRequest(const QByteArray &data);
    void readFileRequest(const QString &filename,int intervalMs);

private slots:
    void initForm();            //初始化窗体数据
    void initConfig();          //初始化配置文件
    void saveConfig();          //保存配置文件
    void readData(const QByteArray &data);            //读取串口数据
    void sendData();            //发送串口数据
    void sendData(QString data);//发送串口数据带参数
    void changeEnable(bool b);  //改变状态

private slots:
    void on_btnOpen_clicked();
    void on_btnStopShow_clicked();
    void on_btnSendCount_clicked();
    void on_btnReceiveCount_clicked();

    void on_btnClear_clicked();
    void on_ckAutoSend_stateChanged(int arg1);
    void on_ckAutoSave_stateChanged(int arg1);
};

#endif // FRMCOMTOOL_H
