# NAME: Angela Wu
# EMAIL: angelawu.123456789@gmail.com
# ID: 604763501

CC = gcc
CFLAGS = -Wall -Wextra -g
.SILENT:

default:
	$(CC) $(CFLAGS) -lz -o lab1b-server lab1b-server.c
	$(CC) $(CFLAGS) -lz -o lab1b-client lab1b-client.c

server:
	$(CC) $(CFLAGS) -lz -o lab1b-server lab1b-server.c

client:
	$(CC) $(CFLAGS) -lz -o lab1b-client lab1b-client.c

dist: default
	tar -czf lab1b-604763501.tar.gz lab1b-server.c lab1b-client.c Makefile README

clean: dist
	rm -rf lab1b-server lab1b-client lab1b-604763501.tar.gz
