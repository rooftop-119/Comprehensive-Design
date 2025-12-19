#ifndef HEAD_H
#define HEAD_H

#include "global.h"
#include "appconfig.h"
#include "appdata.h"

struct DataFrame {
    QDateTime timestamp;
    QByteArray rawData;
};

#endif // HEAD_H
