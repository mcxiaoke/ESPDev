#!/usr/bin/env bash
unset http_proxy
unset https_proxy
unset all_proxy

if [[ "$2x" == "releasex" ]]; then
    RELEASE_FLAG="-release"
else
    RELEASE_FLAG=""
fi

BIN_FILE=".pio/build/nodemcuv2${RELEASE_FLAG}/firmware.bin"
FS_FILE=".pio/build/nodemcuv2${RELEASE_FLAG}/spiffs.bin"
echo "===================="
echo "Uploading firmware to ESP device......"
echo "BIN_FILE: ${BIN_FILE}"
echo "FS_FILE: ${FS_FILE}"
if [[ "$1" == "-s"* ]]; then
    platformio run -e nodemcuv2${RELEASE_FLAG} -t buildfs && curl --progress-bar -v -i -F "firmware=@${FS_FILE}" http://192.168.1.116/update | tee
elif [[ "$1" == "-f"* ]]; then
    platformio run -e nodemcuv2${RELEASE_FLAG} && curl --progress-bar -v -i -F "firmware=@${BIN_FILE}" http://192.168.1.116/update | tee
else
    echo "Usage: $0 [-f|-s] (firmware|filesystem)"
fi
