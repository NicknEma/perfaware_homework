#!/usr/bin/bash
temp=${1##*/}; output=${temp%.c}
clang $1 -o $output $2 -Wall -Wextra -pedantic
