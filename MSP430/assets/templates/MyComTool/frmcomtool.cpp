#include "frmcomtool.h"
#include "ui_frmcomtool.h"
#include <QMessageBox>
#include <QDebug>
#include <algorithm>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTimer>
#include <cmath>
#include <QFormLayout>
#include <QRegularExpression>

frmComTool::frmComTool(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::frmComTool)
    , serial(new QSerialPort(this))
    , chart(new QChart())
    , tempSeries(new QLineSeries())
    , voltageSeries(new QLineSeries())
    , axisX(new QValueAxis())
    , axisY1(new QValueAxis())
    , axisY2(new QValueAxis())
    , maxDataPoints(200)
    , timeCounter(0)
    , timeInterval(0.05)
    , sineWaveCounter(0)
    , generateSineWave(false)
{
    ui->setupUi(this);

    initSerial();
    initDashboard();
    initChart();

    connect(serial, &QSerialPort::readyRead, this, &frmComTool::onReadyRead);

    // 设置定时器用于生成正弦波数据
    QTimer *sineWaveTimer = new QTimer(this);
    connect(sineWaveTimer, &QTimer::timeout, this, &frmComTool::generateSineWaveData);
    sineWaveTimer->start(50);
}

frmComTool::~frmComTool()
{
    if (serial->isOpen()) {
        serial->close();
    }
    delete ui;
}

void frmComTool::initSerial()
{
    // 获取可用串口
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        ui->comboBoxPort->addItem(port.portName());
    }

    // 设置波特率选项
    QStringList baudRates = {"9600", "19200", "38400", "57600", "115200"};
    ui->comboBoxBaud->addItems(baudRates);
    ui->comboBoxBaud->setCurrentText("115200");

    connect(ui->pushButtonOpen, &QPushButton::clicked, this, &frmComTool::onOpenPort);

    // 清空数据按钮
    connect(ui->pushButtonClear, &QPushButton::clicked, this, [this]() {
        tempDataPoints.clear();
        voltageDataPoints.clear();
        timeCounter = 0;
        sineWaveCounter = 0;
        tempSeries->clear();
        voltageSeries->clear();
    });
}

void frmComTool::initDashboard()
{
    // 创建温度仪表盘组
    QGroupBox *tempGroup = new QGroupBox("温度监测");
    tempGroup->setStyleSheet("QGroupBox { font-weight: bold; color: blue; }");
    QVBoxLayout *tempLayout = new QVBoxLayout(tempGroup);

    // 温度圆形仪表盘
    tempDial = new QDial();
    tempDial->setRange(0, 100);
    tempDial->setValue(0);
    tempDial->setNotchesVisible(true);
    tempDial->setNotchTarget(10.0);
    tempDial->setWrapping(false);
    tempDial->setFixedSize(120, 120);
    tempDial->setStyleSheet(
        "QDial {"
        "    background-color: qconicalgradient(cx:0.5, cy:0.5, angle:90, stop:0.1 #0055ff, stop:0.5 #00aaff, stop:0.9 #0055ff);"
        "    border: 2px solid #0044cc;"
        "    border-radius: 60px;"
        "}"
        "QDial::chunk {"
        "    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00aaff, stop:1 #0055ff);"
        "}"
        );

    tempLabel = new QLabel("0°C");
    tempLabel->setAlignment(Qt::AlignCenter);
    tempLabel->setStyleSheet("QLabel { color: blue; font-size: 14px; font-weight: bold; }");

    lcdTemp = new QLCDNumber();
    lcdTemp->setDigitCount(6);
    lcdTemp->setSegmentStyle(QLCDNumber::Filled);
    lcdTemp->display("0.0");
    lcdTemp->setStyleSheet("color: blue; background-color: black; border: 2px solid blue; border-radius: 5px;");
    lcdTemp->setFixedHeight(40);

    tempLayout->addWidget(tempDial, 0, Qt::AlignCenter);
    tempLayout->addWidget(tempLabel);
    tempLayout->addWidget(lcdTemp);

    // 创建电压仪表盘组
    QGroupBox *voltageGroup = new QGroupBox("电压监测");
    voltageGroup->setStyleSheet("QGroupBox { font-weight: bold; color: green; }");
    QVBoxLayout *voltageLayout = new QVBoxLayout(voltageGroup);

    // 电压圆形仪表盘
    voltageDial = new QDial();
    voltageDial->setRange(0, 10);
    voltageDial->setValue(0);
    voltageDial->setNotchesVisible(true);
    voltageDial->setNotchTarget(1.0);
    voltageDial->setWrapping(false);
    voltageDial->setFixedSize(120, 120);
    voltageDial->setStyleSheet(
        "QDial {"
        "    background-color: qconicalgradient(cx:0.5, cy:0.5, angle:90, stop:0.1 #00cc00, stop:0.5 #66ff66, stop:0.9 #00cc00);"
        "    border: 2px solid #009900;"
        "    border-radius: 60px;"
        "}"
        "QDial::chunk {"
        "    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #66ff66, stop:1 #00cc00);"
        "}"
        );

    voltageLabel = new QLabel("0V");
    voltageLabel->setAlignment(Qt::AlignCenter);
    voltageLabel->setStyleSheet("QLabel { color: green; font-size: 14px; font-weight: bold; }");

    lcdVoltage = new QLCDNumber();
    lcdVoltage->setDigitCount(6);
    lcdVoltage->setSegmentStyle(QLCDNumber::Filled);
    lcdVoltage->display("0.0");
    lcdVoltage->setStyleSheet("color: green; background-color: black; border: 2px solid green; border-radius: 5px;");
    lcdVoltage->setFixedHeight(40);

    voltageLayout->addWidget(voltageDial, 0, Qt::AlignCenter);
    voltageLayout->addWidget(voltageLabel);
    voltageLayout->addWidget(lcdVoltage);

    // 控制面板
    QGroupBox *controlGroup = new QGroupBox("数据生成控制");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);

    sineWaveCheck = new QCheckBox("启用正弦波生成");
    sineWaveCheck->setStyleSheet("QCheckBox { color: #444; font-weight: bold; }");
    connect(sineWaveCheck, &QCheckBox::toggled, this, &frmComTool::onSineWaveToggle);

    curveDataButton = new QPushButton("生成曲线测试数据");
    curveDataButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3d8b40;"
        "}"
        );
    connect(curveDataButton, &QPushButton::clicked, this, &frmComTool::onGenerateCurveData);

    controlLayout->addWidget(sineWaveCheck);
    controlLayout->addWidget(curveDataButton);

    // 将仪表盘添加到主布局
    QHBoxLayout *dashboardLayout = new QHBoxLayout();
    dashboardLayout->addWidget(tempGroup);
    dashboardLayout->addWidget(voltageGroup);
    dashboardLayout->addWidget(controlGroup);

    // 插入到主布局的顶部
    ui->verticalLayout->insertLayout(0, dashboardLayout);
}

void frmComTool::initChart()
{
    // 设置系列属性 - 使用更平滑的曲线
    tempSeries->setName("温度 (°C)");
    tempSeries->setColor(QColor(0, 100, 255));
    QPen tempPen(QColor(0, 100, 255), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    tempSeries->setPen(tempPen);

    voltageSeries->setName("电压 (V)");
    voltageSeries->setColor(QColor(0, 150, 0));
    QPen voltagePen(QColor(0, 150, 0), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    voltageSeries->setPen(voltagePen);

    // 设置图表
    chart->addSeries(tempSeries);
    chart->addSeries(voltageSeries);
    chart->setTitle("温度 & 电压实时监测曲线");
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->setBackgroundBrush(QBrush(QColor(245, 245, 245)));
    chart->setTitleBrush(QBrush(QColor(60, 60, 60)));

    // 设置X轴（时间轴）- 实时滚动
    axisX->setTitleText("时间 (s)");
    axisX->setLabelFormat("%.1f");
    axisX->setRange(0, maxDataPoints * timeInterval);
    axisX->setTickCount(10);
    axisX->setLabelsColor(QColor(80, 80, 80));
    axisX->setTitleBrush(QBrush(QColor(80, 80, 80)));
    chart->addAxis(axisX, Qt::AlignBottom);
    tempSeries->attachAxis(axisX);
    voltageSeries->attachAxis(axisX);

    // 设置左侧Y轴（温度）
    axisY1->setTitleText("温度 (°C)");
    axisY1->setLabelFormat("%.1f");
    axisY1->setRange(0, 50);
    axisY1->setTickCount(6);
    axisY1->setLinePenColor(QColor(0, 100, 255));
    axisY1->setLabelsColor(QColor(0, 100, 255));
    axisY1->setTitleBrush(QBrush(QColor(0, 100, 255)));
    chart->addAxis(axisY1, Qt::AlignLeft);
    tempSeries->attachAxis(axisY1);

    // 设置右侧Y轴（电压）
    axisY2->setTitleText("电压 (V)");
    axisY2->setLabelFormat("%.1f");
    axisY2->setRange(0, 5);
    axisY2->setTickCount(6);
    axisY2->setLinePenColor(QColor(0, 150, 0));
    axisY2->setLabelsColor(QColor(0, 150, 0));
    axisY2->setTitleBrush(QBrush(QColor(0, 150, 0)));
    chart->addAxis(axisY2, Qt::AlignRight);
    voltageSeries->attachAxis(axisY2);

    // 将图表添加到界面
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(800, 400);
    chartView->setStyleSheet("QChartView { background-color: white; border: 1px solid #ddd; border-radius: 5px; }");
    ui->verticalLayout->addWidget(chartView);
}

void frmComTool::onOpenPort()
{
    if (serial->isOpen()) {
        serial->close();
        ui->pushButtonOpen->setText("打开串口");
        ui->comboBoxPort->setEnabled(true);
        ui->comboBoxBaud->setEnabled(true);
    } else {
        serial->setPortName(ui->comboBoxPort->currentText());
        serial->setBaudRate(ui->comboBoxBaud->currentText().toInt());
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);

        if (serial->open(QIODevice::ReadWrite)) {
            ui->pushButtonOpen->setText("关闭串口");
            ui->comboBoxPort->setEnabled(false);
            ui->comboBoxBaud->setEnabled(false);
            // 清空数据
            tempDataPoints.clear();
            voltageDataPoints.clear();
            timeCounter = 0;
            sineWaveCounter = 0;
            tempSeries->clear();
            voltageSeries->clear();
        } else {
            QMessageBox::critical(this, "错误", "无法打开串口！");
        }
    }
}

    void frmComTool::onReadyRead()
    {
        QByteArray data = serial->readAll();
        QString dataStr = QString::fromUtf8(data).trimmed();

        qDebug() << "收到数据:" << dataStr;

        // 支持多种分隔符：逗号、空格、分号、制表符
        QRegularExpression re("[,;\\s\\t]+");
        QStringList values = dataStr.split(re, Qt::SkipEmptyParts);

        qDebug() << "解析后的数值列表:" << values;

        if (values.size() >= 2) {
            // 双数值模式：温度,电压
            bool ok1, ok2;
            double temperature = values[0].toDouble(&ok1);
            double voltage = values[1].toDouble(&ok2);

            if (ok1 && ok2) {
                onNewData(temperature, voltage);
                return;
            }
        }

        if (values.size() == 1) {
            // 单数值模式：同时用作温度和电压
            bool ok;
            double value = values[0].toDouble(&ok);
            if (ok) {
                onNewData(value, value);
                return;
            }
        }

        qDebug() << "无法解析的数据格式";
    }

    void frmComTool::onNewData(double temperature, double voltage)
    {
        // 更新时间计数器
        timeCounter += timeInterval;

        // 添加新数据点
        tempDataPoints.append(QPointF(timeCounter, temperature));
        voltageDataPoints.append(QPointF(timeCounter, voltage));

        // 保持数据点数不超过最大值 - 实现滚动效果
        while (tempDataPoints.size() > maxDataPoints) {
            tempDataPoints.removeFirst();
            voltageDataPoints.removeFirst();
        }

        // 更新图表
        updateChart();

        // 更新仪表盘
        updateDashboard(temperature, voltage);
    }

    void frmComTool::updateChart()
    {
        // 确保有数据可以显示
        if (tempDataPoints.isEmpty() || voltageDataPoints.isEmpty()) {
            return;
        }

        // 更新系列数据
        tempSeries->replace(tempDataPoints);
        voltageSeries->replace(voltageDataPoints);

        // 实现X轴滚动效果
        double windowSize = maxDataPoints * timeInterval;
        double currentMaxTime = timeCounter;
        double currentMinTime = qMax(0.0, currentMaxTime - windowSize);

        axisX->setRange(currentMinTime, currentMaxTime);

        // 自动调整Y轴范围
        autoAdjustYAxis();
    }

    void frmComTool::autoAdjustYAxis()
    {
        if (tempDataPoints.isEmpty()) return;

        // 找到温度数据的范围
        double minTemp = tempDataPoints.first().y();
        double maxTemp = tempDataPoints.first().y();
        for (const QPointF &point : tempDataPoints) {
            minTemp = qMin(minTemp, point.y());
            maxTemp = qMax(maxTemp, point.y());
        }

        // 找到电压数据的范围
        double minVoltage = voltageDataPoints.first().y();
        double maxVoltage = voltageDataPoints.first().y();
        for (const QPointF &point : voltageDataPoints) {
            minVoltage = qMin(minVoltage, point.y());
            maxVoltage = qMax(maxVoltage, point.y());
        }

        // 添加10%的边距
        double tempMargin = (maxTemp - minTemp) * 0.1;
        double voltageMargin = (maxVoltage - minVoltage) * 0.1;

        // 防止所有值相同时边距为0
        if (tempMargin == 0) tempMargin = 1;
        if (voltageMargin == 0) voltageMargin = 0.5;

        // 更新Y轴范围
        axisY1->setRange(minTemp - tempMargin, maxTemp + tempMargin);
        axisY2->setRange(minVoltage - voltageMargin, maxVoltage + voltageMargin);

        qDebug() << "温度Y轴范围:" << minTemp - tempMargin << "到" << maxTemp + tempMargin;
        qDebug() << "电压Y轴范围:" << minVoltage - voltageMargin << "到" << maxVoltage + voltageMargin;
    }

    void frmComTool::updateDashboard(double temperature, double voltage)
    {
        // 更新LCD显示
        lcdTemp->display(QString::number(temperature, 'f', 1));
        lcdVoltage->display(QString::number(voltage, 'f', 2));

        // 更新圆形仪表盘
        tempDial->setValue(static_cast<int>(temperature));
        voltageDial->setValue(static_cast<int>(voltage * 2)); // 电压范围0-5V，乘以2适配0-10的刻度

        // 更新标签
        tempLabel->setText(QString("%1°C").arg(temperature, 0, 'f', 1));
        voltageLabel->setText(QString("%1V").arg(voltage, 0, 'f', 2));

        // 根据数值范围改变颜色
        QString tempStyle = tempDial->styleSheet();
        if (temperature > 35) {
            tempStyle = tempStyle.replace("stop:0.1 #0055ff, stop:0.5 #00aaff, stop:0.9 #0055ff",
                                          "stop:0.1 #ff5500, stop:0.5 #ffaa00, stop:0.9 #ff5500");
        } else if (temperature > 25) {
            tempStyle = tempStyle.replace("stop:0.1 #0055ff, stop:0.5 #00aaff, stop:0.9 #0055ff",
                                          "stop:0.1 #ffaa00, stop:0.5 #ffff00, stop:0.9 #ffaa00");
        } else {
            tempStyle = tempStyle.replace("stop:0.1 #ff5500, stop:0.5 #ffaa00, stop:0.9 #ff5500",
                                          "stop:0.1 #0055ff, stop:0.5 #00aaff, stop:0.9 #0055ff")
                            .replace("stop:0.1 #ffaa00, stop:0.5 #ffff00, stop:0.9 #ffaa00",
                                     "stop:0.1 #0055ff, stop:0.5 #00aaff, stop:0.9 #0055ff");
        }
        tempDial->setStyleSheet(tempStyle);
    }

    void frmComTool::onSineWaveToggle(bool checked)
    {
        generateSineWave = checked;
        if (checked) {
            sineWaveCounter = 0;
            // 清空现有数据，开始新的正弦波
            tempDataPoints.clear();
            voltageDataPoints.clear();
            timeCounter = 0;
        }
    }

    void frmComTool::generateSineWaveData()
    {
        if (!generateSineWave) return;

        // 生成正弦波数据
        double temperature = 25 + 10 * sin(sineWaveCounter); // 温度在15-35°C之间波动
        double voltage = 2.5 + 2 * sin(sineWaveCounter * 0.5 + 1); // 电压在0.5-4.5V之间波动，相位偏移

        // 模拟从串口接收数据
        onNewData(temperature, voltage);

        sineWaveCounter += 0.1; // 控制正弦波频率
    }

    void frmComTool::onGenerateCurveData()
    {
        // 生成一段曲线测试数据
        for (int i = 0; i < 50; i++) {
            double x = i * 0.1;
            double temperature = 25 + 10 * sin(x);
            double voltage = 2.5 + 2 * cos(x * 2);
            onNewData(temperature, voltage);
        }
    }
