#ifndef CLICKABLEBUTTON_H
#define CLICKABLEBUTTON_H

#include <QPushButton>
#include <QTimer>
#include <QMouseEvent>

class ClickableButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ClickableButton(QWidget *parent = nullptr);
    explicit ClickableButton(const QString &text, QWidget *parent = nullptr);

    // 设置双击间隔时间（毫秒）
    void setDoubleClickInterval(int msec);
    int doubleClickInterval() const;

signals:
    void singleClicked();  // 单击信号
    void doubleClicked();  // 双击信号

protected:
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void handleSingleClick();

private:
    QTimer *m_timer;
    int m_clickCount;
    int m_doubleClickInterval;
};

#endif // CLICKABLEBUTTON_H
