#include "clickablebutton.h"

ClickableButton::ClickableButton(QWidget *parent)
    : QPushButton(parent)
    , m_clickCount(0)
    , m_doubleClickInterval(300)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ClickableButton::handleSingleClick);
}

ClickableButton::ClickableButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
    , m_clickCount(0)
    , m_doubleClickInterval(300)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ClickableButton::handleSingleClick);
}

void ClickableButton::setDoubleClickInterval(int msec)
{
    m_doubleClickInterval = msec;
}

int ClickableButton::doubleClickInterval() const
{
    return m_doubleClickInterval;
}

void ClickableButton::mousePressEvent(QMouseEvent *event)
{
    QPushButton::mousePressEvent(event);

    m_clickCount++;

    if (m_clickCount == 1) {
        // 第一次点击，启动定时器
        m_timer->start(m_doubleClickInterval);
    } else if (m_clickCount == 2) {
        // 第二次点击，停止定时器，触发双击
        m_timer->stop();
        m_clickCount = 0;
        emit doubleClicked();
    }
}

void ClickableButton::handleSingleClick()
{
    if (m_clickCount == 1) {
        emit singleClicked();
    }
    m_clickCount = 0;
}
