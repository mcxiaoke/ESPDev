#!/usr/bin/env bash
BIN_FILE=".pio/build/esp8266/firmware.bin"
curl -v -F "firmware=@${BIN_FILE}" http://192.168.1.101/update
