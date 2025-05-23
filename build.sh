#!/bin/sh

make
./compiler target.c -o target
gcc -o main main.c
