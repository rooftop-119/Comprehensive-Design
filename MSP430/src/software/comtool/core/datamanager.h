#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include "logger.h"
#include "databuffer.h"
#include "view/datavisualizer.h"
#include "storage/csvwriter.h"

class DataManager : public QObject {
    Q_OBJECT
public:
    explicit DataManager(QObject* parent = nullptr);
    ~DataManager();

    void initialize(QCustomPlot* plot,
                    QLCDNumber* tLcd, QLCDNumber* vLcd,
                    QDial* tDial, QDial* vDial);

    void startRecording(bool enableStorage);
    void stopRecording();
    void pauseDisplay();
    void resumeDisplay();
    void clearChart();
    void updatePaintConf();
    void updateLCDs(double v,double t);

public slots:
    void onSampleReceived(const Sample& sample);
    void onSamplesReceived(const QVector<Sample>& samples);
    void onSampleReceivedDirect(const Sample& sample);

signals:
    void log(Logger::Type type, const QString& msg, bool clear = false);
    void bufferStatusChanged(int used, int total);
    void recordingStarted();
    void fileRotated();

private:
    DataBuffer* m_buffer = nullptr;
    DataVisualizer *m_visualizer = nullptr;
    CSVWriter *m_writer = nullptr;

    bool m_isRecording = false;
    bool m_isPaused = false;
    bool m_enableStorage = true;
};

#endif
