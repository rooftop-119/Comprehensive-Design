#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QTextEdit>
#include <QTextCursor>
#include <QDateTime>
#include <QColor>

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
    int maxLines;

    struct LogMeta {
        const char* prefix;
        QColor color;
    };
};

#endif // LOGGER_H
