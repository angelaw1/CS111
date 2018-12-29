# NAME: Angela Wu
# EMAIL: angelawu.123456789@gmail.com
# ID: 604763501

CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -lmraa -lm -std=gnu99
.SILENT:

default: build

build: tls tcp

tcp:
	$(CC) $(CFLAGS) -o lab4c_tcp lab4c_tcp.c

tls:
	$(CC) $(CFLAGS) -lssl -lcrypto -o lab4c_tls lab4c_tls.c


dist: build
	tar -czf lab4c-604763501.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README

clean:
	rm -rf lab4c_tcp lab4c_tls lab4c-604763501.tar.gz