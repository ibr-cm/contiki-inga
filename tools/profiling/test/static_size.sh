#!/bin/bash

echo -n "Static size in RAM of ${1}: "

avr-size --format=SysV "${1}" | egrep '^.data|^.bss' | tr -s ' ' | cut -d " " -f2 | tr '\n' '+' | sed 's/$/0\n/' | bc
