# NAME: Angela Wu
# EMAIL: angelawu.123456789@gmail.com
# ID: 604763501

CC = gcc
CFLAGS = -Wall -Wextra -g -lpthread -lprofiler -ltcmalloc
.SILENT:

default:
	$(CC) $(CFLAGS) -o lab2_list lab2_list.c SortedList.c

build:
	$(CC) $(CFLAGS) -o lab2_list lab2_list.c SortedList.c

tests:
	rm -rf *.csv
	./sample.sh

profile:
	LD_PRELOAD=/usr/lib64/libprofiler.so CPUPROFILE=profile.prof ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text lab2_list profile.prof > profile.out
	pprof --list=modify_list_spin lab2_list profile.prof >> profile.out
	rm -f profile.prof

graphs:
	./lab2_list.gp

dist: build tests graphs
	tar -czf lab2b-604763501.tar.gz sample.sh SortedList.h SortedList.c lab2_list.c profile.out Makefile *.gp *.csv *.png README

clean:
	rm -rf lab2_list lab2b-604763501.tar.gz