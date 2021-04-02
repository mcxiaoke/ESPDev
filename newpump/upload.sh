#!/usr/bin/env bash
BIN_FILE=".pio/build/nodemcuv2-release/firmware.bin"
FS_FILE=".pio/build/nodemcuv2-release/spiffs.bin"
echo "===================="
echo "Uploading firmware to ESP device......"
if [[ "$1" == "-s"* ]]; then
    platformio run --target buildfs && curl --progress-bar -v -i -F "firmware=@${FS_FILE}" http://192.168.1.116/update | tee
elif [[ "$1" == "-f"* ]]; then
    platformio run && curl --progress-bar -v -i -F "firmware=@${BIN_FILE}" http://192.168.1.116/update | tee
else
    echo "Usage: $0 [-f|-s] (firmware|filesystem)"
fi