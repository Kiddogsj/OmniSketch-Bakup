#!/bin/bash

if [ ! -e build ]; then
    if [ `mkdir build &>/dev/null` ]; then
        echo \e[31mCan not make directory build\e[0m
        exit 1
    fi
fi

if [ ! -e bin ]; then
    if [ `mkdir bin &>/dev/null` ]; then
        echo \e[31mCan not make directory bin
        exit 1
    fi
fi

cd build

cmake .. && make

if [ $? == 0 ]; then
    if [ `mv PcapParser/parser ../bin/ &>/dev/null` ]; then
        echo \e[31mCan not move parser into the build directory\e[0m
        exit 2
    fi
    if [ `mv driver ../bin/ &>/dev/null` ]; then
        echo \e[31mCan not move driver into the build directory\e[0m
        exit 2
    fi
else
    echo \e[31mBuild Failed\e[0m
    exit 3
fi

exit 0

