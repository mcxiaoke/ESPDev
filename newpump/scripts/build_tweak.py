#!/usr/bin/env python3
from subprocess import call, check_output
from datetime import datetime
import os
import sys

Import("env")

print("------ Pre Build Script ------")

date = datetime.now().strftime("%Y%m%d")
revision = check_output(
    ["git", "rev-parse", "--short", "HEAD"]).strip().decode('utf8')
build_version = "{}-{}".format(date, revision)

print(date, revision, build_version)

env.Append(CPPDEFINES=[
    ("BUILD_VERSION", build_version)
])

# -std=c++14 is called -std=c++1y in old gcc versions (at least 4.9 and older)
# xtensa-esp32-elf-g++ (crosstool-NG crosstool-ng-1.22.0-80-g6c4433a) 5.2.0
env.Replace(CXXFLAGS=[
    "-fno-rtti",
    "-std=c++1y"
])

build_flags = env.ParseFlags(env['BUILD_FLAGS'])
print(build_flags.get("CPPDEFINES"))
print(build_flags.get("CXXFLAGS"))
