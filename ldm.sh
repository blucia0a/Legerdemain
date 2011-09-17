#!/bin/bash

LDM=/sampa/home/blucia/cvsandbox/Legerdemain/libs/libldm.so
LDM_OTHERS=/sampa/home/blucia/cvsandbox/Legerdemain/libs/thread_constructor.so

LD_LIBRARY_PATH=/sampa/home/blucia/cvsandbox/libdwarf/libdwarf LD_PRELOAD=${LDM_OTHERS}:${LDM} $@
