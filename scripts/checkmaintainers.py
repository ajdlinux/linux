#!/usr/bin/env python3

# checkmaintainers.py - enforce the format of the MAINTAINERS file
#
# Copyright (C) 2017 IBM Corporation
# Author: Andrew Donnellan <andrew.donnellan@au1.ibm.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.

import re
import glob

# Rules to enforce:
# * Titles in alphabetical order [except THE REST]
# * Consistent spacing (1 blank line between components)
# * F:, N:, X: entries actually need to match a file
# * all other entries should be in syntax correct format
# * no invalid line types

LINE_SPLIT_REGEX = re.compile(r'(?P<type>[PMRLWBCQTSFNXK]):\t(?P<content>\S.*)')
TYPE_REGEX = re.compile(r'^[PMRLWBCQTSFNXK]:')
START_LINE = '\t\t-----------------------------------\n'

VALID_S_VALS = ('Supported', 'Maintained', 'Odd Fixes', 'Orphan', 'Obsolete',
                'Buried alive in reporters')

def split_line(line):
    match = LINE_SPLIT_REGEX.match(line)
    if match:
        return match.groups()
    else:
        return ('', line)

def validate_S(val):
    if val not in VALID_S_VALS:
        return "Invalid S: value"

def validate_file_glob(glob_str):
    files = glob.glob(glob_str)
    if not files:
        return "Glob pattern doesn't match any files"
    
def validate_title(val, last_title):
    if val == '':
        return (None, last_title)
    if TYPE_REGEX.match(val):
        return ("Incorrect indentation", last_title)
    elif val[1] == ":":
        return ("Invalid type '{}'".format(val[1]), last_title)
#    if val.lower() < last_title.lower() and val != "THE REST": # TODO: This doesn't cope with numerals, and it's very noisy right now
#        return ("Incorrectly sorted (last title: {})".format(last_title), last_title)
    return (None, val)
    
def main():
    f = open("MAINTAINERS", "r")
    base_line_no = 1
    while next(f) != START_LINE:
        base_line_no += 1
    last_title = ''
    for idx, line in enumerate(f):
        line_no = base_line_no + idx + 1
        line_type, line_val = split_line(line[:-1])
        result = None
        if line_type == 'S':
            result = validate_S(line_val)
        elif line_type == '':
            result, last_title = validate_title(line_val, last_title)
        elif line_type == 'F' or line_type == 'X':
            result = validate_file_glob(line_val)
        if result:
            print("Line {}: {}\n\t{}".format(line_no, result, line[:-1]))

if __name__ == '__main__':
    main()
