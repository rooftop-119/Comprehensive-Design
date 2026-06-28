#!/bin/sh
set -x

# No argument: start DSP and open the app_host menu.
# With arguments: keep command-line modes for board debugging.
# Usage: $0 [smoke|audio seconds|record output.wav seconds|file input.wav output.wav]

INTERACTIVE=0
if [ $# -eq 0 ]; then
    INTERACTIVE=1
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

    if [ $INTERACTIVE -eq 1 ]; then
        ./app_host
    else
        ./app_host DSP "$@"
    fi
    APP_STATUS=$?
    ./slaveloader shutdown DSP
    SHUTDOWN_STATUS=$?
    if [ $SHUTDOWN_STATUS -ne 0 ]; then
        echo "warning: slaveloader shutdown failed: $SHUTDOWN_STATUS"
    fi
    exit $APP_STATUS
fi
