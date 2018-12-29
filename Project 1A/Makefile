# NAME: Angela Wu
# EMAIL: angelawu.123456789@gmail.com
# ID: 604763501

CC = gcc
CFLAGS = -Wall -Wextra -g
.SILENT:

default:
	$(CC) $(CFLAGS) -o lab1a lab1a.c

dist: default
	tar -czf lab1a-604763501.tar.gz lab1a.c Makefile README

clean: dist
	rm -rf lab1a lab1a-604763501.tar.gz
