# NAME: Angela Wu
# EMAIL: angelawu.123456789@gmail.com
# ID: 604763501

CC = gcc
CFLAGS = -Wall -Wextra -g -lmraa -lm -std=gnu99
.SILENT:

default:
	$(CC) $(CFLAGS) -o lab4b lab4b.c

check: default test
	if [ -s final.txt ]; then \
		echo "Test cases failed"; \
	else \
		echo "Test cases succeeded"; \
	fi;
	cat final.txt
	rm -f final.txt

test:
	{ sleep 2; echo "STOP"; sleep 2; echo "START"; sleep 2; echo "OFF"; } | ./lab4b --log=temp1.txt;
	egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9]+\.[0-9]\>' temp1.txt >> temp2.txt
	grep 'STOP' temp1.txt >> temp3.txt
	grep 'START' temp1.txt >> temp4.txt
	grep 'OFF' temp1.txt >> temp5.txt
	touch final.txt
	if ! [ -s temp2.txt ]; then \
		echo "Failed Test 1" >> final.txt; \
	fi; \
	if ! [ -s temp3.txt ]; then \
		echo "Failed Test 2" >> final.txt; \
	fi; \
	if ! [ -s temp4.txt ]; then \
		echo "Failed Test 3" >> final.txt; \
	fi; \
	if ! [ -s temp5.txt ]; then \
		echo "Failed Test 4" >> final.txt; \
	fi;			
	rm -f temp1.txt temp2.txt temp3.txt temp4.txt temp5.txt

dist: default
	tar -czf lab4b-604763501.tar.gz lab4b.c Makefile README

clean: dist
	rm -rf lab4b lab4b-604763501.tar.gz

