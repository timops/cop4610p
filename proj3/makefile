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
CFLAGS = -Wall -Wextra -g -lpthread

all: matrix.x producer-consumer.x

matrix.x : matrix.c
	$(CC) $(CFLAGS) matrix.c -o matrix.x

producer-consumer.x : producer-consumer.c
	$(CC) $(CFLAGS) producer-consumer.c -o producer-consumer.x

clean:
	rm -f matrix.x producer-consumer.x
