#!/usr/bin/env python3
from subprocess import call, check_output
from datetime import datetime
import shutil
import os
import sys

is_build_env = False

try:
    Import("env")
    is_build_env = True
except:
    print("Not build environment")


print("------ Pre Build Script ------")


def process_os_envs():
    envs = env['ENV']
    for (k, v) in envs.items():
        print('{}={}'.format(k, v))
    print(type(envs), dir(envs))
    print('WIFI_PASS', 'WIFI_PASS' in envs)
    print('MQTT_PASS', 'MQTT_PASS' in envs)
    print('BLYNK_AUTH', 'BLYNK_AUTH' in envs)

    if 'WIFI_SSID' in envs:
        env.Append(CPPDEFINES=[
            ("WIFI_SSID", envs['WIFI_SSID']),
            ('WIFI_PASS', envs['WIFI_PASS'])
        ])

    if 'MQTT_SERVER' in envs:
        env.Append(CPPDEFINES=[
            ("MQTT_SERVER", envs['MQTT_SERVER']),
            ('MQTT_PORT', envs['MQTT_PORT']),
            ('MQTT_USER', envs['MQTT_USER']),
            ('MQTT_PASS', envs['MQTT_PASS'])
        ])

    if 'BLYNK_AUTH' in envs:
        env.Append(CPPDEFINES=[
            ("BLYNK_AUTH", envs['BLYNK_AUTH']),
            ('BLYNK_HOST', envs['BLYNK_HOST']),
            ('BLYNK_PORT', envs['BLYNK_PORT'])
        ])

    if 'WX_REPORT_URL' in envs:
        env.Append(CPPDEFINES=[
            ("WX_REPORT_URL", envs['WX_REPORT_URL'])
        ])


def process_version():
    build_time = datetime.now().strftime("%Y%m%d%H%M")
    git_rev = check_output(
        ["git", "rev-parse", "--short", "HEAD"]).strip().decode('utf8')
    build_version = "{}-{}".format(build_time, git_rev)
    print(build_version)
    env.Append(CPPDEFINES=[
        ("BUILD_REV", build_version)
    ])


def process_build():
    # -std=c++14 is called -std=c++1y in old gcc versions (at least 4.9 and older)
    # xtensa-esp32-elf-g++ (crosstool-NG crosstool-ng-1.22.0-80-g6c4433a) 5.2.0
    env.Replace(CXXFLAGS=[
        "-fno-rtti",
        "-std=c++14"
    ])


def print_envs():
    build_flags = env.ParseFlags(env['BUILD_FLAGS'])
    print(build_flags.get("CPPDEFINES"))
    print(build_flags.get("CXXFLAGS"))


if is_build_env:
    # process_os_envs()
    process_version()
    process_build()
    print_envs()
