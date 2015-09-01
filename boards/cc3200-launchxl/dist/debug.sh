#!/bin/sh

# Start in-circuit debugging on this board using openocd
#

# @author Attilio Dona' <@attiliodona>

CFGDIR=$1
ELFFILE=$2

cd $CFGDIR

# run GDB
arm-none-eabi-gdb -tui -command=gdbinit $ELFFILE
