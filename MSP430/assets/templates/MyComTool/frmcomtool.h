#define FRMCOMTOOL_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QVector>
#include <QtCharts>
#include <QLCDNumber>
#include <QProgressBar>
#include <QLabel>
#include <QCheckBox>
#include <QDial>

QT_BEGIN_NAMESPACE
namespace Ui { class frmComTool; }
QT_END_NAMESPACE

class frmComTool : public QWidget
{
    Q_OBJECT

public:
    explicit frmComTool(QWidget *parent = nullptr);
    ~frmComTool();

private slots:
    void onOpenPort();
    void onReadyRead();
    void onNewData(double temperature, double voltage);
    void onSineWaveToggle(bool checked);
    void onGenerateCurveData();

private:
    Ui::frmComTool *ui;
    QSerialPort *serial;
    QChart *chart;

    // 双Y轴系列
    QLineSeries *tempSeries;    // 温度系列
    QLineSeries *voltageSeries; // 电压系列

    // 双Y轴
    QValueAxis *axisX;
    QValueAxis *axisY1;    // 左侧Y轴（温度）
    QValueAxis *axisY2;    // 右侧Y轴（电压）

    // 数据缓冲区
    QVector<QPointF> tempDataPoints;
    QVector<QPointF> voltageDataPoints;

    // 圆形仪表盘
    QDial *tempDial;
    QDial *voltageDial;
    QLabel *tempLabel;
    QLabel *voltageLabel;
    QLCDNumber *lcdTemp;
    QLCDNumber *lcdVoltage;

    // 正弦波生成
    QCheckBox *sineWaveCheck;
    QPushButton *curveDataButton;
    double sineWaveCounter;
    bool generateSineWave;

    // 图表参数
    int maxDataPoints;
    double timeCounter;
    double timeInterval;

    void initChart();
    void initSerial();
    void initDashboard();
    void updateChart();
    void updateDashboard(double temperature, double voltage);
    void generateSineWaveData();
    void autoAdjustYAxis();
};

