#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QTextEdit>
#include <QTextCursor>
#include <QDateTime>
#include <QColor>
#include "api/appconfig.h"

typedef struct Sample
{
    double timestamp;
    int seq = 0;

    double voltage = 0.0;
    double temperature = 0.0;

    bool hasVoltage = false;
    bool hasTemperature = false;
}Sample;

class Logger : public QObject
{
    Q_OBJECT
public:

    enum class Type {
        SerialSend,
        SerialRecv,
        System,
        File,
        Info
    };
    Q_ENUM(Type)

    explicit Logger(QTextEdit* board, QObject* parent = nullptr);

    void support(QObject* o);

public slots:
    void log(Logger::Type type, const QString& msg, bool clear = false);

private:
    QTextEdit* board;
    int maxLines = AppConfig::logger_max_lines;

    struct LogMeta {
        const char* prefix;
        QColor color;
    };
};

#endif // LOGGER_H
