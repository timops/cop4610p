#
# makefile
# Tim Green
# 3/20/14
# version 1.0
#
# Project 3 - Matrix Multiplication, Producer-Consumer
#
# NOTE: remove -g from CFLAGS to disable debugging information in executable

CC = gcc-4.7
CFLAGS = -Wall -Wextra -g

all: clone.x

clone.x : clone.c
	$(CC) $(CFLAGS) clone.c -o clone.x

clean:
	rm -f clone.x
