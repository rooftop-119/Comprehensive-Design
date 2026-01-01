#include "logger.h"
#include <QRegularExpression>

Logger::Logger(QTextEdit* board, QObject *parent)
    : QObject(parent),
    board(board)
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
    QTextCursor trimCursor(board->document());
    int lineCount = board->document()->lineCount();  // 更准确的行数统计
    if (lineCount > maxLines) {
        trimCursor.movePosition(QTextCursor::Start);
        trimCursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lineCount - maxLines);
        trimCursor.removeSelectedText();
        if (trimCursor.atBlockStart()) {
            trimCursor.deleteChar(); // 删除多余的空行
        }
    }

    // ==================== 关键：实现自动下滑 ====================
    // 方法1：推荐（最可靠，尊重用户是否手动滚动）
    QScrollBar *vScrollBar = board->verticalScrollBar();
    bool shouldAutoScroll = (vScrollBar->value() == vScrollBar->maximum() ||
                             vScrollBar->value() >= vScrollBar->maximum() - 10);  // 容差

    // 强制移动光标到末尾（确保插入在最后）
    cursor.movePosition(QTextCursor::End);
    board->setTextCursor(cursor);

    if (shouldAutoScroll) {
        // 方式A：直接设置滚动条到底（立即生效）
        vScrollBar->setValue(vScrollBar->maximum());

        // 或者方式B：使用 ensureCursorVisible（更平滑，但有时延迟）
        // board->ensureCursorVisible();
    }
}
