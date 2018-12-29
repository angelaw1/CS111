// NAME: Angela Wu
// EMAIL: angelawu.123456789@gmail.com
// ID: 604763501

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#define EXIT_SUCCESS		0			// successful run
#define	EXIT_FAIL			1			// invalid command-line parameter or system call error
#define	EXIT_OTHER			2			// other failures




long long numthread = -1;
long long iterations = -1;
int opt_yield = 0;
int opt_sync = 0;
char sync_type;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int spin_lock = 0;
int compare_swap_lock = 0;

// prints syscall error message and exits
void syscall_error(void) {
	fprintf(stderr, "Error number: %d %s\n", errno, strerror(errno));
	exit(EXIT_FAIL);
}

void commandline_error(void) {
	fprintf(stderr, "Correct Usage: ./lab2_add --threads=# --iterations=# [--yield] [--sync=msc]\n");
	exit(EXIT_FAIL);
}

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
    	sched_yield();
    *pointer = sum;
}

void add_mutex(long long *pointer, long long value) {
	if (pthread_mutex_lock(&lock) != 0) {
		syscall_error();
	}
    add(pointer, value);
    if (pthread_mutex_unlock(&lock) != 0) {
    	syscall_error();
    }
}

void add_spin_lock(long long *pointer, long long value) {
	while (__sync_lock_test_and_set(&spin_lock, 1) == 1)
		;
	add(pointer, value);
    __sync_lock_release(&spin_lock);
}

void add_compare_swap(long long *pointer, long long value) {
	long long expected, newVal;
	for (;;) {
		expected = *pointer;
		newVal = expected + value;
		if (opt_yield)
			sched_yield();
		if (__sync_val_compare_and_swap(pointer, expected, newVal) == expected)
			return;
	}	
}

void *thread_func(void *counter) {
	for (long long i = 0; i != iterations; i++) {
		if (opt_sync) {
			if (sync_type == 'm')
				add_mutex(counter, 1);
			else if (sync_type == 's')
				add_spin_lock(counter, 1);
			else if (sync_type == 'c')
				add_compare_swap(counter, 1);
		}
		else
			add(counter, 1);
	}
	for (long long i = 0; i != iterations; i++) {
		if (opt_sync) {
			if (sync_type == 'm')
				add_mutex(counter, -1);
			else if (sync_type == 's')
				add_spin_lock(counter, -1);
			else if (sync_type == 'c')
				add_compare_swap(counter, -1);
		}
		else
			add(counter, -1);
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv) {

	long long counter = 0;
	int opt_ret;

	if (argc < 3) {
		commandline_error();
	}

	struct option longopts[] = {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", no_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	// parse options
	while((opt_ret = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
		switch(opt_ret) {

			case 't':
				numthread = atoll(optarg);
				break;

			case 'i':
				iterations = atoll(optarg);
				break;

			case 'y':
				opt_yield = 1;
				break;

			case 's':
				if (strlen(optarg) != 1) {
					commandline_error();
				}
				opt_sync = 1;
				sync_type = optarg[0];
				if (sync_type != 'm' && sync_type != 's' && sync_type != 'c') {
					commandline_error();
				}
				break;

			default:
				commandline_error();
				break;

		}
	}

	// checks if threads and interations option was used
	if (numthread == -1 || iterations == -1) {
		commandline_error();
	} 

	pthread_t threads[numthread];

	// get the starting time for the run
	struct timespec begin, end;
	if (clock_gettime(CLOCK_REALTIME, &begin) == -1) {
		syscall_error();
	}

	for (long long i = 0; i != numthread; i++) {
		if (pthread_create(&threads[i], NULL, thread_func, &counter)) {
			syscall_error();
		}
	}

	for (long long i = 0; i != numthread; i++) {
		if (pthread_join(threads[i], NULL)) {
			syscall_error();
		}
	}

	if (clock_gettime(CLOCK_REALTIME, &end) == -1) {
		syscall_error();
	}

	long long elasped_ns = end.tv_nsec - begin.tv_nsec;
	long long total_ops = numthread*iterations*2;

	char* testname;
	if (!opt_yield && !opt_sync)
		testname = "add-none";
	else if (opt_yield && !opt_sync) {
		testname = "add-yield-none";
	}
	else if (!opt_yield && opt_sync) {
		if (sync_type == 'm')
			testname = "add-m";
		else if (sync_type == 's')
			testname = "add-s";
		else if (sync_type == 'c')
			testname = "add-c";
	}
	else {
		if (sync_type == 'm')
			testname = "add-yield-m";
		else if (sync_type == 's')
			testname = "add-yield-s";
		else if (sync_type == 'c')
			testname = "add-yield-c";
	}

	printf("%s,%lld,%lld,%lld,%lld,%lld,%lld\n", 
		testname, numthread, iterations, total_ops,
		elasped_ns, elasped_ns/total_ops, counter);

	//pthread_exit(NULL)
	exit(EXIT_SUCCESS);
}