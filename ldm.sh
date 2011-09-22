#!/bin/bash

LDM=/sampa/home/blucia/cvsandbox/Legerdemain/libs/libldm.so
LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain/libs
LDM_OTHERS=/sampa/home/blucia/cvsandbox/Legerdemain/libs/thread_constructor.so
LIBDWARF=/sampa/home/blucia/cvsandbox/libdwarf/libdwarf 
DRP=/sampa/home/blucia/cvsandbox/DRProbe/drprobe

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DRP:$LDMDIR:$LIBDWARF LD_PRELOAD=${LDM_OTHERS}:${LDM_PRELOAD}:${LDM_CLIENTLIBS}:${LDM} $@
