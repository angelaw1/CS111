# NAME: Angela Wu
# EMAIL: angelawu.123456789@gmail.com
# ID: 604763501

CC = gcc
CFLAGS = -Wall -Wextra -g -lpthread
.SILENT:

default:
	$(CC) $(CFLAGS) -o lab2_add lab2_add.c 
	$(CC) $(CFLAGS) -o lab2_list lab2_list.c SortedList.c

build:
	$(CC) $(CFLAGS) -o lab2_add lab2_add.c 
	$(CC) $(CFLAGS) -o lab2_list lab2_list.c SortedList.c

tests:
	rm -rf *.csv
	./sample.sh

graphs:
	./lab2_add.gp
	./lab2_list.gp

dist: build tests graphs
	tar -czf lab2a-604763501.tar.gz sample.sh lab2_add.c SortedList.h SortedList.c lab2_list.c Makefile *.gp *.csv *.png README

clean:
	rm -rf lab2_add lab2_list lab2a-604763501.tar.gz
