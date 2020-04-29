#!/usr/bin/env python3

import sys

in_str = sys.argv[1]

# in_str = "56 8B 74 24 08 8B 4E 08  8B 41 14 3B 41 18 72 07  ??"

in_str = in_str.lower()

def is_hex(c):
    return '0' <= c <= '9' or 'a' <= c <= 'f'

def hexnum(c):
    if '0' <= c <= '9':
        return ord(c) - ord('0')
    elif 'a' <= c <= 'f':
        return ord(c) - ord('a') + 10
    else:
        raise Exception("Illegal hex char '" + c + "'")

sig = ""
mask=""

i = 0
while i<len(in_str):
    c = in_str[i]
    if c.isspace():
        i += 1
        continue

    if in_str[i:i+2] == "??":
        sig += "\\x00"
        mask += "?"
        i += 2
        continue

    n1 = hexnum(c)
    n2 = hexnum(in_str[i + 1])
    i += 2

    n = 16*n1 + n2

    sig += "\\x%02X" % n
    mask += "x"

print("CREATE_NORMAL_CALLABLE_SIGNATURE(lua_some_func, return_type, \"%s\", \"%s\", 0, lua_State*, other_args)" % (sig, mask))
