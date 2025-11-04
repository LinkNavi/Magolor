#!/bin/bash
echo "=== Checking generated functions ==="
grep "\.globl" hallo.s

echo ""
echo "=== Main function (first 60 lines) ==="
sed -n '/^main:/,/^\.L_main_epilogue:/p' hallo.s | head -60

echo ""
echo "=== Running with GDB ==="
gdb -batch -ex 'run' -ex 'bt' -ex 'info registers' ./program 2>&1 | tail -30
