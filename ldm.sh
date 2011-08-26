#!/bin/bash

LDM=/sampa/home/blucia/cvsandbox/Legerdemain/libs/libldm.so

LD_LIBRARY_PATH=../libdwarf/libdwarf LD_PRELOAD=${LDM} $@
