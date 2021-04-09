#!/usr/bin/env python3
from subprocess import call, check_output
from datetime import datetime
import os
import sys

in_pio_build = False
in_ci_build = False

print("------ Code Check Script ------")

try:
    Import("env", "projenv")
    in_pio_build = True
    print("Pio build")
except:
    pass

try:
    print('CI:', env['CI'])
    print('TRAVIS:', env['TRAVIS'])
    in_ci_build = True
    print('CI build')
except:
    pass


def code_check(source, target, env):
    print("** === Code Check === **")
    print("** Starting cppcheck...")
    with open('build/cppcheck.log', 'w') as f:
        f.write('\n---{}---\n'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S')))
        f.flush()
        call(["cppcheck", "src", "lib", "--force",
              "--enable=warning,style,missingInclude"], stderr=f, stdout=f)
        f.write('\n---{}---\n'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S')))
    print("** Finished cppcheck...")
    print("** Starting cpplint...")
    with open('build/cpplint.log', 'w') as f:
        f.write('\n---{}---\n'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S')))
        f.flush()
        call(["cpplint", "--extensions=ino,cpp,h", "--filter=-legal/copyright,-runtime/int,-runtime/threadsafe_fn,-build/include,-build/header_guard,-whitespace",
              "--linelength=120", "--recursive", "lib", "src"], stderr=f, stdout=f)
        f.write('\n---{}---\n'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S')))
    print("** Finished cpplint...")


# my_flags = env.ParseFlags(env['BUILD_FLAGS'])
# defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}
# print(env)
# print(env.Dump())

#  built in targets: (buildprog, size, upload, program, buildfs, uploadfs, uploadfsota)
if not (in_pio_build or in_ci_build):
    env.AddPostAction("buildprog", code_check)
    # env.Replace(PROGNAME="firmware_%s" % env["BUILD_TYPE"])
# env.AddPostAction(.....)

# see http://docs.platformio.org/en/latest/projectconf/advanced_scripting.html#before-pre-and-after-post-actions
# env.Replace(PROGNAME="firmware_%s" % defines.get("VERSION"))
# env.Replace(PROGNAME="firmware_%s" % env['BOARD'])

if in_pio_build:
    print("========== POST ENV ==========")
    from pprint import pprint
    # print(env.Dump())
    # print(projenv.Dump())
    print('========== BUILD_FLAGS ==========')
    print(projenv['BUILD_FLAGS'])
    print('========== CCFLAGS ==========')
    print(projenv['CCFLAGS'])
    print('========== CFLAGS ==========')
    print(projenv['CFLAGS'])
    print('========== CXXFLAGS ==========')
    print(projenv['CXXFLAGS'])
    print('========== LINKFLAGS ==========')
    print(projenv['LINKFLAGS'])
    print('========== CPPDEFINES ==========')
    print(projenv['CPPDEFINES'])

if __name__ == "__main__":
    code_check(None, None, None)
