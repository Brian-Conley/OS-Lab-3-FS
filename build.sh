#!/bin/bash
cp floppya.img floppya.img.bak
rm -f filesys floppya.img
gcc -o filesys filesys.c
curl -o floppya.img https://timoneilu.github.io/teaching/cs426/labs/lab3/floppya.img
