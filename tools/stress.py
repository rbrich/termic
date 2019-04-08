#!/usr/bin/env python3
#
# Prints to stdout very fast.
#
# Optimal time:
#   time ./stress.py >/dev/null
# Actual time:
#   time ./stress.py

for i in range(1000000):
    print('\033[31;1m', i, '\033[0m')
