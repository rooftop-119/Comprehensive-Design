#include "frmcomtool.h"
#include "ui_frmcomtool.h"

// 核心模块
#include "core/datamanager.h"
#include "transport/serialtransport.h"
#include "transport/filetransport.h"
#include "core/commander.h"
#include "util/framedispatcher.h"
#include "util/framefactory.h"
#include "logger.h"
#include "storage/filemanager.h"
#include "storage/csvreader.h"
#include "view/chartconfig.h"

// Qt组件
#include <qcustomplot.h>
#include <QTimer>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QThread>

#include "api/qthelper.h"
#include "appconfig.h"
#include "component/clickablebutton.h"

frmComTool::frmComTool(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::frmComTool)
{
    ui->setupUi(this);

    initializeUI();
    initializeModules();
    initializeConnections();
    loadHistoryFiles();

    updateUIState(WorkMode::Idle);

    QtHelper::setFormInCenter(this);
}

frmComTool::~frmComTool() {
    // 确保线程安全退出
    if (m_serialThread && m_serialThread->isRunning()) {
        m_serialThread->quit();
        m_serialThread->wait(3000);
    }

    delete ui;
}

void frmComTool::closeEvent(QCloseEvent *event) {
    if (m_workMode != WorkMode::Idle) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认退出",
            "数据采集正在进行中，确定要退出吗？",
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }

        // 停止所有操作
        if (m_dataManager) {
            m_dataManager->stopRecording();
        }
        if (m_serialTransport && m_serialTransport->isOpen()) {
            m_serialTransport->close();
        }
    }

    saveConfig();
    event->accept();
}

// ============================================================
// 初始化函数
// ============================================================

void frmComTool::initializeUI() {
    // 获取UI组件引用
    m_plot = ui->widgetChartContainer;
    m_tempLcd = ui->lcdTemp;
    m_voltLcd = ui->lcdVoltage;
    m_tempDial = ui->tempDial;
    m_voltDial = ui->voltageDial;

    // 初始化串口配置选项
    initializeSerialPortOptions();

    // 初始化图表与仪表盘样式
    initializeDashboardStyles();

    // 初始化统计显示
    ui->labelSendCount->setText("发送字节: 0");
    ui->labelRecvCount->setText("接收字节: 0");
}

void frmComTool::initializeModules() {
    // ========== 创建 Logger ==========
    m_logger = new Logger(ui->textEditLog, this);
    m_logger->log(Logger::Type::System, "系统初始化中...");

    // ========== 创建传输层 ==========
    m_serialTransport = new SerialTransport();
    m_fileTransport = new FileTransport(this);
    m_serialTransport->setDataShow(AppConfig::DataShow);

    // ========== 创建工具类 ==========
    m_factory = new FrameFactory();
    m_dispatcher = new FrameDispatcher(this);
    m_fileManager = new FileManager(this);

    // ========== 创建命令管理器 ==========
    m_commander = new Commander(m_factory, this);

    // ========== 创建数据管理器 ==========
    m_dataManager = new DataManager(this);
    m_dataManager->initialize(m_plot, m_tempLcd, m_voltLcd, m_tempDial, m_voltDial);
    m_plot->setBufferDevicePixelRatio(devicePixelRatioF());

    // 配置数据管理器
    // ChartConfig chartConfig;
    // chartConfig.updateInterval = 50;
    // chartConfig.timeWindow = 10.0;
    // chartConfig.batchSize = 10;
    // m_dataManager->setChartConfig(chartConfig);
    // m_dataManager->setOutputDir("logs");

    // ========== 创建串口线程 ==========
    m_serialThread = new QThread(this);
    m_serialTransport->moveToThread(m_serialThread);

    // 线程结束时清理
    connect(m_serialThread, &QThread::finished,
            m_serialTransport, &QObject::deleteLater);

    m_serialThread->start();

    m_logger->log(Logger::Type::System, "系统初始化完成");
}

void frmComTool::initializeConnections() {
    // ========== Logger 连接（统一日志） ==========
    m_logger->support(m_serialTransport);
    m_logger->support(m_fileTransport);
    m_logger->support(m_dispatcher);
    m_logger->support(m_commander);
    m_logger->support(m_dataManager);
    m_logger->support(m_fileManager);

    //connect(ui->pushButtonDraw,&ClickableButton::singleClicked,this,&frmComTool::on_pushButtonDraw_singleClicked);
    //connect(ui->pushButtonDraw,&ClickableButton::doubleClicked,this,&frmComTool::on_pushButtonDraw_doubleClicked);

    // ========== 串口传输 → 分发器 ==========
    connect(m_serialTransport, &SerialTransport::frameReceived,
            m_dispatcher, &FrameDispatcher::dispatch,
            Qt::QueuedConnection);

    // ========== 文件传输 → 分发器 ==========
    connect(m_fileTransport, &FileTransport::frameReceived,
            m_dispatcher, &FrameDispatcher::dispatch,
            Qt::QueuedConnection);

    // ========== 分发器 → 数据管理器（数据帧） ==========
    connect(m_dispatcher, &FrameDispatcher::sampleReady,
            m_dataManager, &DataManager::onSampleReceived,
            Qt::QueuedConnection);

    // ========== 分发器 → 命令管理器（应答帧） ==========
    connect(m_dispatcher, &FrameDispatcher::gotAns,
            m_commander, &Commander::onAnswerReceived,
            Qt::QueuedConnection);

    // ========== 命令管理器 → 串口传输（发送命令） ==========
    connect(m_commander, &Commander::sendData,
            m_serialTransport, &SerialTransport::write,
            Qt::QueuedConnection);

    // ========== 命令管理器 → UI（命令结果） ==========
    connect(m_commander, &Commander::commandCompleted,
            this, &frmComTool::onCommandCompleted,
            Qt::QueuedConnection);
    connect(m_commander, &Commander::commandTimeout,
            this, &frmComTool::onCommandTimeout,
            Qt::QueuedConnection);

    // ========== 串口状态变化 ==========
    connect(m_serialTransport, &SerialTransport::stateChanged,
            this, &frmComTool::onSerialStateChanged,
            Qt::QueuedConnection);
    connect(m_serialTransport, &SerialTransport::errorOccurred,
            this, &frmComTool::onSerialError,
            Qt::QueuedConnection);

    // ========== 文件回放控制 ==========
    connect(m_fileTransport, &FileTransport::playbackFinished,
            this, &frmComTool::onPlaybackFinished);

    // ========== 数据统计更新 ==========
    QTimer *statsTimer = new QTimer(this);
    connect(statsTimer, &QTimer::timeout,
            this, &frmComTool::onBytesStatisticsUpdate);
    statsTimer->start(500);  // 每500ms更新一次统计

    // ========== 缓冲区状态更新 ==========
    connect(m_dataManager, &DataManager::bufferStatusChanged,
            this, &frmComTool::onBufferStatusUpdate);
    connect(m_dataManager, &DataManager::fileRotated,
            this, &frmComTool::loadHistoryFiles);

    // ========== 串口配置变化时保存 ==========
    connect(ui->comboBoxPort, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &frmComTool::saveConfig);
    connect(ui->comboBoxBaud, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &frmComTool::saveConfig);
    connect(ui->comboBoxDataBits, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &frmComTool::saveConfig);
    connect(ui->comboBoxParity, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &frmComTool::saveConfig);
    connect(ui->comboBoxStopBits, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &frmComTool::saveConfig);
}

void frmComTool::initializeSerialPortOptions() {
    // 获取可用串口
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        ui->comboBoxPort->addItem(port.portName());
    }
    ui->comboBoxPort->setCurrentText(AppConfig::PortName);

    // 波特率
    QStringList baudRates = {"9600", "19200", "38400", "57600", "115200"};
    ui->comboBoxBaud->addItems(baudRates);
    ui->comboBoxBaud->setCurrentText(QString::number(AppConfig::BaudRate));

    // 数据位
    QStringList dataBits = {"5", "6", "7", "8"};
    ui->comboBoxDataBits->addItems(dataBits);
    ui->comboBoxDataBits->setCurrentText(QString::number(AppConfig::DataBit));

    // 校验位
    QStringList parityList = {"无", "奇", "偶"};
#ifdef Q_OS_WIN
    parityList << "标志";
#endif
    parityList << "空格";
    ui->comboBoxParity->addItems(parityList);
    ui->comboBoxParity->setCurrentText(AppConfig::Parity);

    // 停止位
    QStringList stopBitsList = {"1"};
#ifdef Q_OS_WIN
    stopBitsList << "1.5";
#endif
    stopBitsList << "2";
    ui->comboBoxStopBits->addItems(stopBitsList);
    ui->comboBoxStopBits->setCurrentText(QString::number(AppConfig::StopBit));

    // 采样模式
    QStringList sampleModes = {"电压采样", "温度采样", "同时采样", "历史回放"};
    ui->comboBoxSampleMode->addItems(sampleModes);
    ui->comboBoxSampleMode->setCurrentIndex(3);  // 默认历史回放
}

void frmComTool::initializeDashboardStyles() {
    // ========== 温度仪表盘 ==========
    m_tempDial->setStyleSheet(
        "QDial {"
        "    background-color: qradialgradient(cx:0.5, cy:0.5, radius:0.5, "
        "        stop:0 #1a237e, stop:0.3 #283593, stop:0.7 #3949ab, stop:1 #1a237e);"
        "    border: 3px solid #3949ab; border-radius: 70px;"
        "}"
        );
    m_tempDial->setRange(-5, 105);
    m_tempDial->setValue(0);
    m_tempDial->setSingleStep(5);   // 每 5°C 一小格
    m_tempDial->setPageStep(20);    // 每 20°C 一大格
    m_tempDial->setNotchesVisible(true);

    conT = connect(m_tempDial, &QDial::valueChanged,this, [=](int value) {
        m_dataManager->updateLCDs(0, value);
    });

    // ========== 温度LCD ==========
    m_tempLcd->setStyleSheet(
        "QLCDNumber {"
        "    color: #29b6f6; background-color: #0d1117;"
        "    border: 2px solid #3949ab; border-radius: 10px; padding: 5px;"
        "}"
        );
    m_tempLcd->setDigitCount(6);
    m_tempLcd->setSegmentStyle(QLCDNumber::Flat);
    m_tempLcd->display("00.00");

    // ========== 电压仪表盘 ==========
    m_voltDial->setStyleSheet(
        "QDial {"
        "    background-color: qradialgradient(cx:0.5, cy:0.5, radius:0.5, "
        "        stop:0 #1b5e20, stop:0.3 #2e7d32, stop:0.7 #388e3c, stop:1 #1b5e20);"
        "    border: 3px solid #388e3c; border-radius: 70px;"
        "}"
        );
    m_voltDial->setRange(0, 5000);
    m_voltDial->setValue(0);
    // 关键设置：控制刻度密度
    m_voltDial->setSingleStep(100);   // 小刻度每 100 单位一条（原先是1，太密了）
    m_voltDial->setPageStep(500);     // 大刻度每 500 单位一条（可选，影响键盘翻页）
    m_voltDial->setNotchesVisible(true);  // 现在刻度就稀疏好看了

    conV = connect(m_voltDial, &QDial::valueChanged,this, [=](int value) {
        m_dataManager->updateLCDs(value, 0);
    });

    // ========== 电压LCD ==========
    m_voltLcd->setStyleSheet(
        "QLCDNumber {"
        "    color: #66bb6a; background-color: #0d1117;"
        "    border: 2px solid #388e3c; border-radius: 10px; padding: 5px;"
        "}"
        );
    m_voltLcd->setDigitCount(6);
    m_voltLcd->setSegmentStyle(QLCDNumber::Flat);
    m_voltLcd->display("0.000");

    // ========== 按钮样式 ==========
    QString buttonBaseStyle =
        "QPushButton { padding: 8px 16px; border-radius: 6px; "
        "font-weight: bold; font-size: 14px; border: none; }"
        "QPushButton:hover { border: 2px solid rgba(255,255,255,0.5); }";

    ui->pushButtonOpen->setStyleSheet(buttonBaseStyle +
                                      "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                                      "stop:0 #2196F3, stop:1 #1976D2); color: white; }");

    ui->pushButtonDraw->setStyleSheet(buttonBaseStyle +
                                      "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                                      "stop:0 #9C27B0, stop:1 #7B1FA2); color: white; }");

    ui->pushButtonSend->setStyleSheet(buttonBaseStyle +
                                      "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                                      "stop:0 #009688, stop:1 #00796B); color: white; }");

    ui->pushButtonClear->setStyleSheet(buttonBaseStyle +
                                       "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                                       "stop:0 #f44336, stop:1 #d32f2f); color: white; }");
}

void frmComTool::loadHistoryFiles() {
    if (!m_fileManager) return;

    QList<FileInfo> recentFiles = m_fileManager->getRecentFiles("logs", 10);

    ui->comboBoxFiles->clear();
    for (const auto& file : recentFiles) {
        QString displayText = QString("%1 (%2 MB)")
        .arg(file.fileName)
            .arg(file.fileSize / 1024.0 / 1024.0, 0, 'f', 2);
        ui->comboBoxFiles->addItem(displayText, file.filePath);
    }

    if (ui->comboBoxFiles->count() == 0) {
        ui->comboBoxFiles->addItem("无历史文件");
        ui->comboBoxFiles->setEnabled(false);
    }
}

// ============================================================
// 状态管理
// ============================================================

void frmComTool::updateUIState(WorkMode mode) {
    m_workMode = mode;

    switch (mode) {
    case WorkMode::Idle:
        ui->comboBoxPort->setEnabled(true);
        ui->comboBoxBaud->setEnabled(true);
        ui->comboBoxDataBits->setEnabled(true);
        ui->comboBoxParity->setEnabled(true);
        ui->comboBoxStopBits->setEnabled(true);
        ui->comboBoxSampleMode->setEnabled(true);
        ui->comboBoxFiles->setEnabled(true);

        ui->pushButtonOpen->setText("打开串口");
        ui->pushButtonOpen->setEnabled(true);
        ui->pushButtonDraw->setText("开始");
        ui->pushButtonDraw->setEnabled(true);
        ui->pushButtonSend->setEnabled(false);

        m_tempDial->setValue(0);
        m_voltDial->setValue(0);
        disconnect(conT);
        disconnect(conV);
        conT = connect(m_tempDial, &QDial::valueChanged,this, [=](int value) {
            m_dataManager->updateLCDs(0, value);
        });
        conV = connect(m_voltDial, &QDial::valueChanged,this, [=](int value) {
            m_dataManager->updateLCDs(value, 0);
        });
        break;

    case WorkMode::SerialSampling:
        ui->comboBoxPort->setEnabled(false);
        ui->comboBoxBaud->setEnabled(false);
        ui->comboBoxDataBits->setEnabled(false);
        ui->comboBoxParity->setEnabled(false);
        ui->comboBoxStopBits->setEnabled(false);
        ui->comboBoxSampleMode->setEnabled(false);
        ui->comboBoxFiles->setEnabled(false);

        ui->pushButtonOpen->setText("关闭串口");
        ui->pushButtonOpen->setEnabled(true);
        ui->pushButtonDraw->setText("暂停(双击关闭)");
        ui->pushButtonDraw->setEnabled(true);
        ui->pushButtonSend->setEnabled(true);

        m_tempDial->setValue(0);
        m_voltDial->setValue(0);
        disconnect(conT);
        disconnect(conV);
        break;

    case WorkMode::FilePlayback:
        ui->comboBoxPort->setEnabled(false);
        ui->comboBoxBaud->setEnabled(false);
        ui->comboBoxDataBits->setEnabled(false);
        ui->comboBoxParity->setEnabled(false);
        ui->comboBoxStopBits->setEnabled(false);
        ui->comboBoxSampleMode->setEnabled(false);
        ui->comboBoxFiles->setEnabled(false);

        ui->pushButtonOpen->setEnabled(false);
        ui->pushButtonDraw->setText("暂停(双击关闭)");
        ui->pushButtonDraw->setEnabled(true);
        ui->pushButtonSend->setEnabled(false);

        m_tempDial->setValue(0);
        m_voltDial->setValue(0);
        disconnect(conT);
        disconnect(conV);
        break;
    }
}

void frmComTool::saveConfig() {
    AppConfig::PortName = ui->comboBoxPort->currentText();
    AppConfig::BaudRate = ui->comboBoxBaud->currentText().toInt();
    AppConfig::DataBit = ui->comboBoxDataBits->currentText().toInt();
    AppConfig::Parity = ui->comboBoxParity->currentText();
    AppConfig::StopBit = ui->comboBoxStopBits->currentText().toDouble();
    AppConfig::writeConfig();
}

QString frmComTool::getSelectedFilePath() const {
    return ui->comboBoxFiles->currentData().toString();
}

// ============================================================
// 槽函数实现
// ============================================================

void frmComTool::on_pushButtonOpen_clicked() {
    if (!m_serialTransport->isOpen()) {
        onOpenSerialPort();
    } else {
        onCloseSerialPort();
    }
}

void frmComTool::onOpenSerialPort() {
    // 配置串口参数
    SerialConfig config;
    config.portName = ui->comboBoxPort->currentText();
    config.baudRate = ui->comboBoxBaud->currentText().toInt();
    config.dataBits = ui->comboBoxDataBits->currentText().toInt();

    // 解析校验位
    QString parity = ui->comboBoxParity->currentText();
    if (parity == "无") config.parity = QSerialPort::NoParity;
    else if (parity == "奇") config.parity = QSerialPort::OddParity;
    else if (parity == "偶") config.parity = QSerialPort::EvenParity;
    else config.parity = QSerialPort::NoParity;

    // 解析停止位
    double stopBits = ui->comboBoxStopBits->currentText().toDouble();
    if (stopBits == 1.0) config.stopBits = QSerialPort::OneStop;
    else if (stopBits == 1.5) config.stopBits = QSerialPort::OneAndHalfStop;
    else config.stopBits = QSerialPort::TwoStop;

    m_serialTransport->setConfig(config);

    // 跨线程调用 open
    QMetaObject::invokeMethod(m_serialTransport, "open",
                              Qt::QueuedConnection);
}

void frmComTool::onCloseSerialPort() {
    // 先停止采样
    if (m_workMode == WorkMode::SerialSampling) {
        onStopSampling();
    }

    // 跨线程调用 close
    QMetaObject::invokeMethod(m_serialTransport, "close",
                              Qt::QueuedConnection);
}

void frmComTool::onSerialStateChanged(ITransport::State state) {
    switch (state) {
    case ITransport::State::Connected:
        updateUIState(WorkMode::Idle);  // 连接后进入空闲状态
        ui->pushButtonOpen->setText("关闭串口");
        ui->pushButtonSend->setEnabled(true);
        ui->pushButtonDraw->setEnabled(true);
        break;

    case ITransport::State::Disconnected:
        updateUIState(WorkMode::Idle);
        break;

    case ITransport::State::Error:
        updateUIState(WorkMode::Idle);
        QMessageBox::warning(this, "串口错误", "串口通信发生错误");
        break;

    default:
        break;
    }
}

void frmComTool::onSerialError(const QString &error) {
    QMessageBox::critical(this, "串口错误", error);
}

void frmComTool::on_pushButtonDraw_doubleClicked() {
    if (m_workMode == WorkMode::Idle) {
        // 判断是串口采样还是文件回放
        if (ui->comboBoxSampleMode->currentIndex() == 3) {
            // 历史回放
            onStartPlayback();
        } else {
            // 串口采样
            onStartSampling();
        }
    } else {
        // 停止当前操作
        if (m_workMode == WorkMode::SerialSampling) {
            onStopSampling();
        } else if (m_workMode == WorkMode::FilePlayback) {
            onStopPlayback();
        }
    }
}

void frmComTool::on_pushButtonDraw_singleClicked() {
    if (m_workMode == WorkMode::Idle) {
        // 判断是串口采样还是文件回放
        if (ui->comboBoxSampleMode->currentIndex() == 3) {
            // 历史回放
            onStartPlayback();
        } else {
            // 串口采样
            onStartSampling();
        }
    } else {
        if (m_workMode == WorkMode::SerialSampling) {
            if(ui->pushButtonDraw->text()=="暂停(双击关闭)"){
                m_dataManager->pauseDisplay();
                ui->pushButtonDraw->setText("恢复");
            }else{
                m_dataManager->resumeDisplay();
                ui->pushButtonDraw->setText("暂停(双击关闭)");
            }
        } else if (m_workMode == WorkMode::FilePlayback) {
            if(ui->pushButtonDraw->text()=="暂停(双击关闭)"){
                m_fileTransport->pause();
                ui->pushButtonDraw->setText("恢复");
            }else{
                m_fileTransport->resume();
                ui->pushButtonDraw->setText("暂停(双击关闭)");
            }
        }
    }
}

void frmComTool::onStartSampling() {
    if (!m_serialTransport->isOpen()) {
        QMessageBox::warning(this, "警告", "请先打开串口");
        return;
    }

    int intervalMs = 150;  // 可以从UI读取
    int sampleMode = ui->comboBoxSampleMode->currentIndex();

    ui->pushButtonDraw->setEnabled(false);  // 等待命令响应

    // 发送相应的启动命令
    switch (sampleMode) {
    case 0:  // 电压采样
        m_commander->sendStartVoltage(intervalMs);
        break;
    case 1:  // 温度采样
        m_commander->sendStartTemp(intervalMs);
        break;
    case 2:  // 同时采样
        m_commander->sendStartBoth(intervalMs);
        break;
    default:
        ui->pushButtonDraw->setEnabled(true);
        break;
    }
}

void frmComTool::onStopSampling() {
    ui->pushButtonDraw->setEnabled(false);  // 等待命令响应
    m_commander->sendStop();
}

void frmComTool::onCommandCompleted(FrameType cmdType, int seq, ErrorType error) {
    ui->pushButtonDraw->setEnabled(true);

    if (error != ErrorType::SUCCESS) {
        QString cmdName;
        switch (cmdType) {
        case FrameType::CmdStop: cmdName = "停止"; break;
        case FrameType::CmdStartVoltage: cmdName = "启动电压采样"; break;
        case FrameType::CmdStartTemp: cmdName = "启动温度采样"; break;
        case FrameType::CmdStartBoth: cmdName = "启动双路采样"; break;
        default: cmdName = "未知命令"; break;
        }

        QMessageBox::warning(this, "命令失败",
                             QString("命令 [%1] 执行失败\n序列号: %2\n错误码: %3")
                                 .arg(cmdName)
                                 .arg(seq)
                                 .arg(static_cast<int>(error)));
        return;
    }

    // 命令执行成功
    switch (cmdType) {
    case FrameType::CmdStartVoltage:
    case FrameType::CmdStartTemp:
    case FrameType::CmdStartBoth:
        // 启动数据记录
        m_dataManager->startRecording(true);
        updateUIState(WorkMode::SerialSampling);
        m_logger->log(Logger::Type::System,
                      QString("开始数据采集 (seq=%1)").arg(seq));
        break;

    case FrameType::CmdStop:
        // 停止数据记录
        m_dataManager->stopRecording();
        updateUIState(WorkMode::Idle);
        ui->pushButtonOpen->setText("关闭串口");  // 串口仍然打开
        ui->pushButtonSend->setEnabled(true);
        ui->pushButtonDraw->setEnabled(true);
        m_logger->log(Logger::Type::System,
                      QString("停止数据采集 (seq=%1)").arg(seq));
        break;

    default:
        break;
    }
}

void frmComTool::onCommandTimeout(FrameType cmdType, int seq) {
    ui->pushButtonDraw->setEnabled(true);

    QString cmdName;
    switch (cmdType) {
    case FrameType::CmdStop: cmdName = "停止"; break;
    case FrameType::CmdStartVoltage: cmdName = "启动电压采样"; break;
    case FrameType::CmdStartTemp: cmdName = "启动温度采样"; break;
    case FrameType::CmdStartBoth: cmdName = "启动双路采样"; break;
    default: cmdName = "未知命令"; break;
    }

    QMessageBox::warning(this, "命令超时",
                         QString("命令 [%1] 响应超时\n序列号: %2\n请检查下位机连接")
                             .arg(cmdName)
                             .arg(seq));

    m_logger->log(Logger::Type::System,
                  QString("命令超时: %1 (seq=%2)").arg(cmdName).arg(seq));
}

void frmComTool::onStartPlayback() {
    QString filePath = getSelectedFilePath();

    if (filePath.isEmpty() || filePath == "无历史文件") {
        QMessageBox::warning(this, "警告", "请选择有效的历史文件");
        return;
    }

    m_fileTransport->setFilePath(filePath);
    m_fileTransport->setPlaybackSpeed(1.0);  // 正常速度
    m_fileTransport->setLoopMode(false);     // 不循环

    if (m_fileTransport->open()) {
        m_dataManager->startRecording(false);
        updateUIState(WorkMode::FilePlayback);
        m_logger->log(Logger::Type::File,
                      QString("开始文件回放: %1").arg(filePath));
    } else {
        QMessageBox::critical(this, "错误", "文件打开失败");
    }
}

void frmComTool::onStopPlayback() {
    m_fileTransport->close();
    m_dataManager->stopRecording();
    updateUIState(WorkMode::Idle);
    m_logger->log(Logger::Type::File, "停止文件回放");
}

void frmComTool::onPlaybackFinished() {
    m_dataManager->stopRecording();
    updateUIState(WorkMode::Idle);

    QMessageBox::information(this, "完成", "文件回放已完成");
    m_logger->log(Logger::Type::File, "文件回放完成");
}

void frmComTool::on_pushButtonSend_clicked() {
    QString text = ui->textEditSend->toPlainText();

    if (text.isEmpty()) {
        ui->textEditSend->setFocus();
        return;
    }

    if (!m_serialTransport->isOpen()) {
        QMessageBox::warning(this, "警告", "串口未打开");
        return;
    }

    QByteArray data = text.toUtf8();

    // 跨线程发送数据
    QMetaObject::invokeMethod(m_serialTransport, "write",
                              Qt::QueuedConnection,
                              Q_ARG(QByteArray, data));

    m_sendCount += data.size();
}

void frmComTool::on_pushButtonClear_clicked() {
    m_logger->log(Logger::Type::System, "", true);  // 清空日志
    m_dataManager->clearChart();  // 清空图表
}

// void frmComTool::on_pushButtonRefreshFiles_clicked() {
//     loadHistoryFiles();
//     m_logger->log(Logger::Type::Info, "文件列表已刷新");
// }

void frmComTool::onRefreshFileList() {
    loadHistoryFiles();
}

void frmComTool::onBytesStatisticsUpdate() {
    if (m_serialTransport) {
        m_receiveCount = m_serialTransport->bytesReceived();
        m_sendCount = m_serialTransport->bytesSent();
    }

    ui->labelSendCount->setText(QString("发送字节: %1").arg(m_sendCount));
    ui->labelRecvCount->setText(QString("接收字节: %1").arg(m_receiveCount));
}

void frmComTool::onBufferStatusUpdate(int used, int capacity) {
    // 可以在状态栏或其他地方显示缓冲区状态
    float percentage = float(used*100)/capacity;
    QString status = QString("缓冲区: %1/%2 (%3%)")
                         .arg(used)
                         .arg(capacity)
                         .arg(percentage, 0, 'f', 1);

    // 如果有状态栏
    // statusBar()->showMessage(status);

    // 或者更新到标签
    ui->labelBufferStatus->setText(status);

    // 如果缓冲区使用率过高，可以显示警告
    if (percentage > 90.0f) {
        m_logger->log(Logger::Type::System,
                      QString("警告：缓冲区使用率过高 (%1%)").arg(percentage, 0, 'f', 1));
    }
}

void frmComTool::on_pushButtonFresh_clicked(){
    AppConfig::readConfig();

    // 获取可用串口
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        ui->comboBoxPort->addItem(port.portName());
    }

    m_serialTransport->setDataShow(AppConfig::DataShow);

    loadHistoryFiles();

    m_dataManager->updatePaintConf();
}
