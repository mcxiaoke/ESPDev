#!/usr/bin/env zsh
idf.py build && idf.py -p /dev/cu.usbserial-0001 flash && idf.py -p /dev/cu.usbserial-0001 monitor
# Ctrl+] to quit monitor
