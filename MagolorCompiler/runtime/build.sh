#!/bin/bash
# Compile the Magolor runtime library

gcc -c runtime.c -o runtime.o -O2 -Wall
ar rcs libmagolor.a runtime.o
rm runtime.o

echo "✓ Runtime library built: libmagolor.a"
