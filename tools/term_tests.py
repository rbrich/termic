#!/usr/bin/env python3

# Replace existing text with different attributes
print("\033[31;1mred green blue normal red\033[21D"
      "\033[32mgreen \033[34mblue \033[mnormal")
print("\033[31;1mred \033[32mgreen \033[34mblue \033[mnormal \033[31;1mred\033[m (attribute replace)")

# Switch to insert mode and back to replace
print("\033[4habcdefghi\b\b\b\b\b\b123\033[4lDEF")
print("abc123DEFghi (insert mode)")

# Erase in line
print('EL 0:123456\033[44m\033[6D\033[0K\033[0m')
print('     ^ blue bg from here')

print('12345678 EL 1\033[6D\033[46m\033[1K\033[0m')
print('       ^ up to here: cyan bg')

print('12345678\033[45m\033[2K\033[0m')
print('^ whole line: magenta bg')
