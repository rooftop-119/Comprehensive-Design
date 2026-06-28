#!/bin/sh
set -x

if [ $# -eq 0 ]; then
    set -- smoke
fi

MODE=$1

if [ "$MODE" = "file" ] && [ $# -lt 2 ]; then
    echo "Usage: $0 file input.wav [output.wav]"
    exit 1
fi

if [ "$MODE" = "record" ] && [ $# -lt 2 ]; then
    echo "Usage: $0 record output.wav [seconds]"
    exit 1
fi

if [ "$MODE" = "record" ]; then
    ./app_host "$@"
else
    ./slaveloader startup DSP server_dsp.xe674
    SLAVE_STATUS=$?
    if [ $SLAVE_STATUS -ne 0 ]; then
        echo "slaveloader startup failed: $SLAVE_STATUS"
        exit $SLAVE_STATUS
    fi

    ./app_host DSP "$@"
    APP_STATUS=$?
    ./slaveloader shutdown DSP
    SHUTDOWN_STATUS=$?
    if [ $SHUTDOWN_STATUS -ne 0 ]; then
        echo "warning: slaveloader shutdown failed: $SHUTDOWN_STATUS"
    fi
    exit $APP_STATUS
fi
