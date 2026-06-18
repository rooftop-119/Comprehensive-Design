#ifndef FRAMEPARSER_H
#define FRAMEPARSER_H

#include <QByteArray>
#include <memory>

#include "logger.h"
#include "entity/baseframe.h"
#include "entity/datavoltageframe.h"
#include "entity/datatempframe.h"
#include "entity/databothframe.h"
#include "entity/ansframe.h"

class FrameParser
{
public:
    std::shared_ptr<BaseFrame> parse(const QByteArray& raw);

private:
    bool checkCRC(const QByteArray& raw,int len);
    int start = 2;
};

#endif // FRAMEPARSER_H
