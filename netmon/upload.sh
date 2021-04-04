#!/usr/bin/env bash
unset http_proxy
unset https_proxy
unset all_proxy
BIN_FILE=".pio/build/esp8266/firmware.bin"
FS_FILE=".pio/build/esp8266/spiffs.bin"
echo "===================="
echo "Uploading firmware to ESP device......"
if [[ "$1" == "-s"* ]]; then
    platformio run --target buildfs && curl --progress-bar -v -i -F "firmware=@${FS_FILE}" http://192.168.1.101/update | tee
elif [[ "$1" == "-f"* ]]; then
    platformio run && curl --progress-bar -v -i -F "firmware=@${BIN_FILE}" http://192.168.1.101/update | tee
else
    echo "Usage: $0 [-f|-s] (firmware|filesystem)"
fi
