#!/usr/bin/env bash
DEBUG_FILE=./.pio/build/nodemcuv2/firmware.bin
RELEASE_FILE=./.pio/build/nodemcuv2-release/firmware.bin
if test -f "$DEBUG_FILE"; then
curl --progress-bar -i -F "firmware=@${DEBUG_FILE}" http://192.168.1.116/update | tee
elif test -f "$RELEASE_FILE"; then
curl --progress-bar -i -F "firmware=@${RELEASE_FILE}" http://192.168.1.116/update | tee
fi
