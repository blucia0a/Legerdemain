#!/bin/bash

LDM=/sampa/home/blucia/cvsandbox/Legerdemain/libldm.so
LD_PRELOAD=${LDM} $@
