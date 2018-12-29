# NAME: Angela Wu
# EMAIL: angelawu.123456789@gmail.com
# ID: 604763501

CC = gcc
CFLAGS = -Wall -Wextra -g
.SILENT:

default:
	$(CC) $(CFLAGS) -o lab0 lab0.c

check: default test1 test2 test3 test4 test5
	if [ -s final.txt ]; then \
		echo "Test cases failed"; \
	else \
		echo "Test cases succeeded"; \
	fi; \
	rm -f final.txt

test1 :
	echo "Hello my name is Angela" > input.txt; \
	./lab0 --input=input.txt --output=out.txt; \
	diff -u input.txt out.txt > res1.txt; \
	touch final.txt; 
	if [ -s res1.txt ]; then \
		echo "Error: input doesn't match output" >> final.txt; \
	fi; \
	rm -f res1.txt input.txt out.txt

test2:
	echo "My name is Angela" > input.txt; \
	./lab0 --random --input=input.txt; \
	return_code=$$?; \
	if [[ $$return_code -ne 1 ]]; then \
		echo "Error: wrong argument case exited with wrong code" >> final.txt; \
	fi; \
	rm -f input.txt

test3:
	./lab0 --input=input.txt; \
	return_code=$$?; \
	if [[ $$return_code != 2 ]]; then \
		echo "Error: missing input case exited with wrong code" >> final.txt; \
	fi
	rm -f input.txt

test4:
	echo "Hello my name is Angela" > input.txt; \
	./lab0 --input=input.txt --catch --segfault; \
	return_code=$$?; \
	if [[ $$return_code != 4 ]]; then \
		echo "Error: catch case exited with wrong code" >> final.txt; \
	fi; \
	rm -f input.txt
test5:
	echo "Hello my name is Angela" > input.txt; \
	touch out.txt; \
	chmod u-w out.txt; \
	./lab0 --input=input.txt --output=out.txt; \
	return_code=$$?; \
	if [[ $$return_code != 3 ]]; then \
		echo "Error: Didn't exit with right code when output file didn't have write permission" >> final.txt; \
	fi; \
	rm -f input.txt out.txt 

dist: default
	tar -czf lab0-604763501.tar.gz lab0.c Makefile *.png README

clean: dist
	rm -rf lab0 lab0-604763501.tar.gz
