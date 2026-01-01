#ifndef FRMCOMTOOL_H
#define FRMCOMTOOL_H

#include <QWidget>
#include <QSerialPortInfo>
#include <QThread>

// 前向声明
class QCustomPlot;
class QLCDNumber;
class QDial;

// 核心模块
class SerialTransport;
class FileTransport;
class Commander;
class DataManager;
class FrameDispatcher;
class FrameFactory;
class Logger;
class FileManager;
class CSVReader;

// 实体头文件
#include "entity/frametype.h"
#include "entity/errortype.h"
#include "transport/itransport.h"

namespace Ui {
class frmComTool;
}

class frmComTool : public QWidget {
    Q_OBJECT

public:
    explicit frmComTool(QWidget *parent = nullptr);
    ~frmComTool();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::frmComTool *ui;

    // UI组件引用
    QCustomPlot *m_plot = nullptr;
    QLCDNumber *m_voltLcd = nullptr;
    QLCDNumber *m_tempLcd = nullptr;
    QDial *m_voltDial = nullptr;
    QDial *m_tempDial = nullptr;

    // 核心业务模块
    SerialTransport *m_serialTransport = nullptr;
    FileTransport *m_fileTransport = nullptr;
    Commander *m_commander = nullptr;
    DataManager *m_dataManager = nullptr;
    FrameDispatcher *m_dispatcher = nullptr;
    FrameFactory *m_factory = nullptr;
    Logger *m_logger = nullptr;
    FileManager *m_fileManager = nullptr;
    CSVReader *m_csvReader = nullptr;

    // 线程
    QThread *m_serialThread = nullptr;

    // 状态变量
    enum class WorkMode {
        Idle,           // 空闲
        SerialSampling, // 串口采样中
        FilePlayback    // 文件回放中
    };

    WorkMode m_workMode = WorkMode::Idle;
    qint64 m_sendCount = 0;
    qint64 m_receiveCount = 0;

private:

    QMetaObject::Connection conV,conT;
    // 初始化函数
    void initializeUI();
    void initializeModules();
    void initializeConnections();
    void initializeSerialPortOptions();
    void initializeDashboardStyles();
    void loadHistoryFiles();

    // 辅助函数
    void updateUIState(WorkMode mode);
    void saveConfig();
    QString getSelectedFilePath() const;

private slots:
    // 串口操作
    void onOpenSerialPort();
    void onCloseSerialPort();
    void onSerialStateChanged(ITransport::State state);
    void onSerialError(const QString &error);

    // 命令操作
    void onStartSampling();
    void onStopSampling();
    void onCommandCompleted(FrameType cmdType, int seq, ErrorType error);
    void onCommandTimeout(FrameType cmdType, int seq);

    // 文件操作
    void onStartPlayback();
    void onStopPlayback();
    void onPlaybackFinished();
    void onRefreshFileList();

    // 数据统计更新
    void onBytesStatisticsUpdate();
    void onBufferStatusUpdate(int used, int capacity);

    // UI按钮槽（自动连接）
    void on_pushButtonOpen_clicked();
    void on_pushButtonSend_clicked();
    void on_pushButtonDraw_singleClicked();
    void on_pushButtonDraw_doubleClicked();
    void on_pushButtonClear_clicked();
    void on_pushButtonFresh_clicked();
    //void on_pushButtonRefreshFiles_clicked();
};

#endif // FRMCOMTOOL_H
