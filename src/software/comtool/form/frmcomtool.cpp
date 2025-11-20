#include "frmcomtool.h"
#include "ui_frmcomtool.h"
#include "qthelper.h"
#include "qthelperdata.h"
#include "qcustomplot.h"
#include "serialworker.h"
#include <QTimer>
#include <QHBoxLayout>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>

frmComTool::frmComTool(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::frmComTool)
{
    ui->setupUi(this);
    this->initForm();
    this->initConfig();
    QtHelper::setFormInCenter(this);
    this->init();
    this->setUpPlot();
}

frmComTool::~frmComTool()
{
    delete ui;
}

void frmComTool::init()
{
    // ==========================
    // 1. 先创建所有 Buffer
    // ==========================
    m_buffer = new DataBuffer(this);
    filebuffer = new FileBuffer(this);

    // ==========================
    // 2. 创建 Logger（必须早创建）
    // ==========================
    logger = new Logger(ui->txtMain, this);


    // ======================================================
    // 3. 创建工作对象（必须在 moveToThread 前完成 new）
    // ======================================================
    worker = new SerialWorker();          // 串口工作线程对象
    fileWriter = new FileWriter(m_buffer); // 文件写入器
    fileReader = new FileReader(this); // 文件读取器（主线程运行）
    filePainter = new FilePainter(m_plot, filebuffer, this);
    serialPainter = new SerialPainter(m_plot, m_buffer, this);


    // ======================================================
    // 4. 创建两个线程
    // ======================================================
    serialThread = new QThread(this);
    fileThread = new QThread(this);


    // ======================================================
    // 5. 将工作对象移入线程
    // ======================================================
    worker->moveToThread(serialThread);
    fileWriter->moveToThread(fileThread);


    // ======================================================
    // 6. 线程结束时安全释放对象
    // ======================================================
    connect(serialThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(fileThread, &QThread::finished, fileWriter, &QObject::deleteLater);

    // 在窗体销毁时退出线程
    connect(this, &QObject::destroyed, serialThread, [=]{
        serialThread->quit();
        serialThread->wait();
    });

    connect(this, &QObject::destroyed, fileThread, [=]{
        fileThread->quit();
        fileThread->wait();
    });


    // ======================================================
    // 7. 设置 signal-slot（线程安全）
    // ======================================================

    // ------ SerialWorker 信号 ------
    connect(this, &frmComTool::openPortRequest, worker, &SerialWorker::openPort);
    connect(this, &frmComTool::closePortRequest, worker, &SerialWorker::closePort);
    connect(this, &frmComTool::writeRequest, worker, &SerialWorker::writeData);

    connect(worker, &SerialWorker::gotData, this, [=](const QByteArray &data){
        readData(data);
    });

    connect(worker, &SerialWorker::portOpened, this, [=]{
        logger->log(Logger::Type::System,
                    QString("串口 %1 打开成功").arg(ui->cboxPortName->currentText()));
        changeEnable(true);
        ui->btnOpen->setText("关闭串口");
    });

    connect(worker, &SerialWorker::portClosed, this, [=]{
        logger->log(Logger::Type::System,"串口已关闭");
        changeEnable(false);
        ui->btnOpen->setText("打开串口");
    });


    // ------ FileWriter 信号 ------
    connect(fileWriter, &FileWriter::fileNameUpdate, this, [=]{
        ui->file->clear();
        ui->file->addItems(getRecentCsvFiles("logs/", 8));
    });

    // 串口关闭时通知写文件器 flush/stop
    connect(worker, &SerialWorker::portClosed, fileWriter, &FileWriter::update);

    // 串口打开时开始写入
    connect(worker, &SerialWorker::portOpened, fileWriter, &FileWriter::start);


    // ------ FileReader（不在线程）------
    connect(this, &frmComTool::readFileRequest, fileReader, &FileReader::start);


    // ------ FilePainter ------
    connect(this, &frmComTool::readFileRequest, filePainter, &FilePainter::start);
    //connect(filebuffer, &FileBuffer::noDataForPlot, filePainter, &FilePainter::stop);
    connect(fileReader,&FileReader::linesRead,filebuffer,&FileBuffer::append);


    // ------ SerialPainter ------
    connect(worker, &SerialWorker::portOpened, serialPainter, &SerialPainter::start);

    // 当串口关闭（UI上显示“打开串口”），停止绘图
    connect(m_buffer, &DataBuffer::noDataForPlot, this, [=]{
        if (ui->btnOpen->text() == "打开串口") {
            serialPainter->stop();
        }
    });


    // ======================================================
    // 8. Logger 依赖注入（必须在 logger 创建之后）
    // ======================================================
    logger->support(m_buffer);
    logger->support(filebuffer);
    logger->support(fileWriter);
    logger->support(fileReader);
    logger->support(filePainter);
    logger->support(serialPainter);
    logger->support(worker);


    // ======================================================
    // 9. 最后启动线程（必须是最后一步）
    // ======================================================
    serialThread->start();
    fileThread->start();
}


void frmComTool::initForm()
{
    m_plot = ui->customPlot;

    comOk = false;
    sleepTime = 10;
    receiveCount = 0;
    sendCount = 0;
    isShow = true;

    ui->cboxSendInterval->addItems(AppData::Intervals);
    ui->cboxData->addItems(AppData::Datas);

    //发送数据
    connect(ui->btnSend, SIGNAL(clicked()), this, SLOT(sendData()));

    ui->tabWidget->setCurrentIndex(0);
    changeEnable(false);

    // connect(ui->file, &QComboBox::currentIndexChanged,
    //         this, &frmComTool::on_files);
    connect(ui->btnClear,&QPushButton::clicked,this,&frmComTool::on_btnClear_clicked);

#ifdef __arm__
    ui->widgetRight->setFixedWidth(280);
#endif
}

void frmComTool::initConfig()
{
    ui->file->clear();
    ui->file->addItems(getRecentCsvFiles("logs/",8));

    QStringList comList;
    for (int i = 1; i <= 10; i++) {
        comList << QString("COM%1").arg(i);
    }

    comList << "ttyUSB0" << "ttyS0" << "ttyS1" << "ttyS2" << "ttyS3" << "ttyS4";
    comList << "ttymxc1" << "ttymxc2" << "ttymxc3" << "ttymxc4";
    comList << "ttySAC1" << "ttySAC2" << "ttySAC3" << "ttySAC4";
    ui->cboxPortName->addItems(comList);
    ui->cboxPortName->setCurrentIndex(ui->cboxPortName->findText(AppConfig::PortName));
    connect(ui->cboxPortName, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    QStringList baudList;
    baudList << "50" << "75" << "100" << "134" << "150" << "200" << "300" << "600" << "1200"
             << "1800" << "2400" << "4800" << "9600" << "14400" << "19200" << "38400"
             << "56000" << "57600" << "76800" << "115200" << "128000" << "256000";

    ui->cboxBaudRate->addItems(baudList);
    ui->cboxBaudRate->setCurrentIndex(ui->cboxBaudRate->findText(QString::number(AppConfig::BaudRate)));
    connect(ui->cboxBaudRate, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    QStringList dataBitsList;
    dataBitsList << "5" << "6" << "7" << "8";

    ui->cboxDataBit->addItems(dataBitsList);
    ui->cboxDataBit->setCurrentIndex(ui->cboxDataBit->findText(QString::number(AppConfig::DataBit)));
    connect(ui->cboxDataBit, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    QStringList parityList;
    parityList << "无" << "奇" << "偶";
#ifdef Q_OS_WIN
    parityList << "标志";
#endif
    parityList << "空格";

    ui->cboxParity->addItems(parityList);
    ui->cboxParity->setCurrentIndex(ui->cboxParity->findText(AppConfig::Parity));
    connect(ui->cboxParity, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    QStringList stopBitsList;
    stopBitsList << "1";
#ifdef Q_OS_WIN
    stopBitsList << "1.5";
#endif
    stopBitsList << "2";

    ui->cboxStopBit->addItems(stopBitsList);
    ui->cboxStopBit->setCurrentIndex(ui->cboxStopBit->findText(QString::number(AppConfig::StopBit)));
    connect(ui->cboxStopBit, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    ui->ckHexSend->setChecked(AppConfig::HexSend);
    connect(ui->ckHexSend, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->ckHexReceive->setChecked(AppConfig::HexReceive);
    connect(ui->ckHexReceive, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->ckDebug->setChecked(AppConfig::Debug);
    connect(ui->ckDebug, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->ckAutoClear->setChecked(AppConfig::AutoClear);
    connect(ui->ckAutoClear, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->ckAutoSend->setChecked(AppConfig::AutoSend);
    connect(ui->ckAutoSend, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->ckAutoSave->setChecked(AppConfig::AutoSave);
    connect(ui->ckAutoSave, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    QStringList sendInterval;
    QStringList saveInterval;
    sendInterval << "100" << "300" << "500";

    for (int i = 1000; i <= 10000; i = i + 1000) {
        sendInterval << QString::number(i);
        saveInterval << QString::number(i);
    }

    ui->cboxSendInterval->addItems(sendInterval);
    ui->cboxSaveInterval->addItems(saveInterval);

    ui->cboxSendInterval->setCurrentIndex(ui->cboxSendInterval->findText(QString::number(AppConfig::SendInterval)));
    connect(ui->cboxSendInterval, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));
    ui->cboxSaveInterval->setCurrentIndex(ui->cboxSaveInterval->findText(QString::number(AppConfig::SaveInterval)));
    connect(ui->cboxSaveInterval, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    // timerSend->setInterval(AppConfig::SendInterval);
    // timerSave->setInterval(AppConfig::SaveInterval);

    // if (AppConfig::AutoSend) {
    //     timerSend->start();
    // }

    // if (AppConfig::AutoSave) {
    //     timerSave->start();
    // }
}

void frmComTool::saveConfig()
{
    AppConfig::PortName = ui->cboxPortName->currentText();
    AppConfig::BaudRate = ui->cboxBaudRate->currentText().toInt();
    AppConfig::DataBit = ui->cboxDataBit->currentText().toInt();
    AppConfig::Parity = ui->cboxParity->currentText();
    AppConfig::StopBit = ui->cboxStopBit->currentText().toDouble();

    AppConfig::HexSend = ui->ckHexSend->isChecked();
    AppConfig::HexReceive = ui->ckHexReceive->isChecked();
    AppConfig::Debug = ui->ckDebug->isChecked();
    AppConfig::AutoClear = ui->ckAutoClear->isChecked();

    AppConfig::AutoSend = ui->ckAutoSend->isChecked();
    AppConfig::AutoSave = ui->ckAutoSave->isChecked();

    int sendInterval = ui->cboxSendInterval->currentText().toInt();
    if (sendInterval != AppConfig::SendInterval) {
        AppConfig::SendInterval = sendInterval;
        // timerSend->setInterval(AppConfig::SendInterval);
    }

    int saveInterval = ui->cboxSaveInterval->currentText().toInt();
    if (saveInterval != AppConfig::SaveInterval) {
        AppConfig::SaveInterval = saveInterval;
        // timerSave->setInterval(AppConfig::SaveInterval);
    }

    AppConfig::Mode = ui->cboxMode->currentText();
    AppConfig::ServerIP = ui->txtServerIP->text().trimmed();
    AppConfig::ServerPort = ui->txtServerPort->text().toInt();
    AppConfig::ListenPort = ui->txtListenPort->text().toInt();
    AppConfig::SleepTime = ui->cboxSleepTime->currentText().toInt();
    AppConfig::AutoConnect = ui->ckAutoConnect->isChecked();

    AppConfig::writeConfig();
}

void frmComTool::changeEnable(bool b)
{
    ui->cboxBaudRate->setEnabled(!b);
    ui->cboxDataBit->setEnabled(!b);
    ui->cboxParity->setEnabled(!b);
    ui->cboxPortName->setEnabled(!b);
    ui->cboxStopBit->setEnabled(!b);
    ui->btnSend->setEnabled(b);
    ui->ckAutoSend->setEnabled(b);
    ui->ckAutoSave->setEnabled(b);
}

QStringList frmComTool::getRecentCsvFiles(const QString &dirPath, int count)
{
    QStringList result;

    QDir dir(dirPath);
    if (!dir.exists())
        return result;

    // 只获取 *.csv 文件
    QStringList filters;
    filters << "*.csv";

    // 获取文件信息列表，按时间排序（Newest First）
    QFileInfoList fileList = dir.entryInfoList(
        filters,
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Time | QDir::Reversed // 默认时间降序：最新在前
        );

    // 只取最近 count 个
    int n = qMin(count, fileList.size());
    for (int i = 0; i < n; i++)
    {
        result << fileList[i].fileName();  // 只返回文件名
    }

    return result;
}

void frmComTool::setUpPlot()
{

    m_plot->setNotAntialiasedElements(QCP::aeAll);
    m_plot->setAntialiasedElements(QCP::aeNone);
    m_plot->setBufferDevicePixelRatio(devicePixelRatioF());

    // 左侧电压曲线
    m_plot->addGraph(m_plot->xAxis, m_plot->yAxis);
    auto voltageGraph = m_plot->graph(0);
    QColor voltageColor(76, 175, 80);              // 绿色 #4CAF50
    QColor voltageFill(76, 175, 80, 40);           // 淡绿色填充（透明 40）

    voltageGraph->setPen(QPen(voltageColor, 1));
    voltageGraph->setBrush(QBrush(voltageFill));   // 填充曲线下方区域
    voltageGraph->setLineStyle(QCPGraph::lsLine);

    m_plot->yAxis->setLabel("Voltage (V)");
    m_plot->yAxis->setRange(0, 5);

    // 右侧温度曲线
    m_plot->yAxis2->setVisible(true);
    m_plot->yAxis2->setLabel("Temperature (°C)");
    m_plot->yAxis2->setRange(0, 120);

    m_plot->addGraph(m_plot->xAxis, m_plot->yAxis2);
    auto tempGraph = m_plot->graph(1);
    QColor tempColor(33, 150, 243);                 // 蓝色 #2196F3
    QColor tempFill(33, 150, 243, 30);              // 浅蓝色填充

    tempGraph->setPen(QPen(tempColor, 1));
    tempGraph->setBrush(QBrush(tempFill));           // 填充曲线区域
    tempGraph->setLineStyle(QCPGraph::lsLine);

    // 时间轴
    QSharedPointer<QCPAxisTickerDateTime> ticker(new QCPAxisTickerDateTime);
    ticker->setDateTimeFormat("hh:mm:ss");
    ticker->setDateTimeSpec(Qt::LocalTime);
    m_plot->xAxis->setTicker(ticker);

    double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    m_plot->xAxis->setRange(now - m_timeWindowSec, now);

    voltageGraph->data()->setAutoSqueeze(true);
    tempGraph->data()->setAutoSqueeze(true);

    m_plot->legend->setVisible(true);
    voltageGraph->setName("Voltage");
    tempGraph->setName("Temperature");

    m_plot->replot();
}

void frmComTool::readData(const QByteArray &data)
{
    // ---- 1. 转为字符串显示 ----
    QString buffer = QtHelperData::byteArrayToHexStr(data);

    // ---- 2. 调试功能 ----
    if (ui->ckDebug->isChecked()) {
        int count = AppData::Keys.count();
        for (int i = 0; i < count; i++) {
            if (buffer.startsWith(AppData::Keys.at(i))) {
                sendData(AppData::Values.at(i));
                break;
            }
        }
    }

    // ---- 4. 接收计数统计 ----
    receiveCount += data.size();
    ui->btnReceiveCount->setText(QString("接收 : %1 字节").arg(receiveCount));

    // ---- 6. 加入数据缓存（带时间戳） ---
    double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    m_buffer->append(now,data);
}

void frmComTool::sendData()
{
    QString str = ui->cboxData->currentText();
    if (str.isEmpty()) {
        ui->cboxData->setFocus();
        return;
    }

    sendData(str);

    if (ui->ckAutoClear->isChecked()) {
        ui->cboxData->setCurrentIndex(-1);
        ui->cboxData->setFocus();
    }
}

void frmComTool::sendData(QString data)
{
    if (!worker || !worker->isOpen() || !worker->isOpen()) {
        logger->log(Logger::Type::SerialSend,"串口未打开，无法发送");
        return;
    }

    QByteArray buffer = data.toUtf8();
    buffer = QtHelperData::hexStrToByteArray(data);

    //调用 SerialWorker 的封装
    emit writeRequest(buffer);

    sendCount += buffer.size();
    ui->btnSendCount->setText(QString("发送 : %1 字节").arg(sendCount));
}

void frmComTool::on_btnOpen_clicked()
{
    if (ui->btnOpen->text() == "打开串口") {
        emit openPortRequest(
            ui->cboxPortName->currentText(),
            ui->cboxBaudRate->currentText().toInt(),
            ui->cboxDataBit->currentText().toInt(),
            ui->cboxParity->currentText(),
            ui->cboxStopBit->currentText().toDouble()
            );
        serialPainter->start();
    } else {
        emit closePortRequest();
    }
}

void frmComTool::on_btnSendCount_clicked()
{
    sendCount = 0;
    ui->btnSendCount->setText("发送 : 0 字节");
}

void frmComTool::on_btnReceiveCount_clicked()
{
    receiveCount = 0;
    ui->btnReceiveCount->setText("接收 : 0 字节");
}

void frmComTool::on_btnStopShow_clicked()
{
    if (ui->btnStopShow->text() == "停止显示") {
        isShow = false;
        ui->btnStopShow->setText("开始显示");
    } else {
        isShow = true;
        ui->btnStopShow->setText("停止显示");
    }
}

void frmComTool::on_btnClear_clicked()
{
    QString text = ui->file->currentText();
    emit readFileRequest(text,250);
}

void frmComTool::on_ckAutoSend_stateChanged(int arg1)
{
    if (arg1 == 0) {
        ui->cboxSendInterval->setEnabled(false);
        // timerSend->stop();
    } else {
        ui->cboxSendInterval->setEnabled(true);
        // timerSend->start();
    }
}

void frmComTool::on_ckAutoSave_stateChanged(int arg1)
{
    if (arg1 == 0) {
        ui->cboxSaveInterval->setEnabled(false);
        // timerSave->stop();
    } else {
        ui->cboxSaveInterval->setEnabled(true);
        // timerSave->start();
    }
}
