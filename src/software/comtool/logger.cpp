#include "logger.h"
#include <QRegularExpression>

Logger::Logger(QTextEdit* board, QObject *parent)
    : QObject(parent),
    board(board),
    maxLines(100)        // 最大行数，可根据需求调
{
}

void Logger::support(QObject* o)
{
    connect(o, SIGNAL(log(Logger::Type,QString,bool)),
            this, SLOT(log(Logger::Type,QString,bool)),
            Qt::QueuedConnection);
}

void Logger::log(Type type, const QString &msg, bool clear)
{
    static const LogMeta logTable[] = {
                                       { "串口发送 >>", QColor("dodgerblue") },
                                       { "串口接收 <<", QColor("gray") },
                                       { "系统进程 >>", QColor("red") },
                                       { "文件交互 >>", QColor("green") },
                                       { "提示信息 >>", QColor(100,184,255) },
                                       };

    if (clear) {
        board->clear();
        return;
    }

    // 分类文本与颜色
    const LogMeta& meta = logTable[static_cast<int>(type)];

    QString prefix = meta.prefix;
    QColor  color  = meta.color;

    // 统一格式的时间戳
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    // ---- 使用 QTextCursor 插入彩色文本（不会污染全局格式） ----
    QTextCursor cursor = board->textCursor();
    cursor.movePosition(QTextCursor::End);

    // time 格式（浅灰）
    QTextCharFormat timeFmt;
    timeFmt.setForeground(QColor("black"));
    cursor.insertText(QString("[%1] ").arg(time), timeFmt);

    // prefix 格式（使用 logTable 的颜色）
    QTextCharFormat prefixFmt;
    prefixFmt.setForeground(meta.color);
    cursor.insertText(QString("[%1] ").arg(meta.prefix), prefixFmt);

    // data 内容（白色）
    QTextCharFormat dataFmt;
    dataFmt.setForeground(QColor("black"));
    cursor.insertText(msg + "\n", dataFmt);

    // ---- 最大行数滚动 ----
    QString content = board->toPlainText();
    int lineCount = content.count('\n');

    if (lineCount > maxLines) {
        QTextCursor c = board->textCursor();
        c.movePosition(QTextCursor::Start);
        c.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,
                       lineCount - maxLines);
        c.removeSelectedText();
        c.deleteChar();  // 删除额外换行
    }
}
