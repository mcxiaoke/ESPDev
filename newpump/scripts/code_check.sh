#!/usr/bin/env bash

echo "** Starting cppcheck.."
cppcheck --force --enable=all src 2>build/cppcheck.log
echo "** Finished cppcheck.."
echo "** Starting cpplint.."
cpplint --recursive --linelength=120 --extensions=ino,c,c++,h++,cxx,hpp,cu,cpp,h,hh,cc,hxx,cuh output=vs7 src 2>build/cpplint.log
echo "** Finished cpplint.."
