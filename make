#!/bin/bash
mkdir -p bin/release
gcc src/*.c -std=c99 -Wall -Wextra -O2 -o bin/release/gomoku-ai
