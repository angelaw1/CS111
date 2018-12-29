# NAME: Angela Wu,Candice Zhang
# EMAIL: angelawu.123456789@gmail.com,candicezhang1997@gmail.com
# ID: 604763501,604757623

CC = gcc
CFLAGS = -Wall -Wextra -g
.SILENT:

default:
	$(CC) $(CFLAGS) -o lab3a lab3a.c

dist: default
	tar -czf lab3a-604763501.tar.gz lab3a.c Makefile README ext2_fs.h

clean: dist
	rm -rf lab3a lab3a-604763501.tar.gz
