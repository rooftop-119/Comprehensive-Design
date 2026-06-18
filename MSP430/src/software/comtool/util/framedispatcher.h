#ifndef FRAMEDISPATCHER_H
#define FRAMEDISPATCHER_H

#include <QObject>
#include <memory>

#include "logger.h"
#include "entity/baseframe.h"
#include "entity/datavoltageframe.h"
#include "entity/datatempframe.h"
#include "entity/databothframe.h"
#include "entity/ansframe.h"
#include "framefactory.h"

class FrameDispatcher : public QObject
{
    Q_OBJECT
public:
    explicit FrameDispatcher(QObject* parent = nullptr);

public slots:
    void dispatch(std::shared_ptr<BaseFrame> frame);

signals:
    void sampleReady(const Sample& sample);
    void gotAns(const AnsFrame& ans);
    void log(Logger::Type type, const QString& msg, bool clear = false);
};

#endif // FRAMEDISPATCHER_H
