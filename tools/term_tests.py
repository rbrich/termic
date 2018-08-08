#!/usr/bin/env python3

# Replace existing text with different attributes
print("\033[31;1mred green blue normal red\033[21D"
      "\033[32mgreen \033[34mblue \033[mnormal")
print("\033[31;1mred \033[32mgreen \033[34mblue \033[mnormal \033[31;1mred\033[m (attribute replace)")

# Switch to insert mode and back to replace
print("\033[4habcdefghi\b\b\b\b\b\b123\033[4lDEF")
print("abc123DEFghi (insert mode)")
