// NAME: Angela Wu
// EMAIL: angelawu.123456789@gmail.com
// ID: 604763501

#include "SortedList.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>


#define EXIT_SUCCESS		0			// successful run
#define	EXIT_FAIL			1			// invalid command-line parameter or system call error
#define	EXIT_OTHER			2			// other failures


long long numthread = 1;
long long iterations = 1;
int opt_yield = 0;
int sync_flag = 0;
char sync_opt;
SortedList_t *head;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int spin_lock = 0;

// signal handler for SIGSEGV
void handler(int sig) {
	if (sig == SIGSEGV) {
    	fprintf(stderr, "Error number: %d %s\n", errno, strerror(errno));
    	exit(EXIT_OTHER);
  	}
}

// prints syscall error message and exits
void syscall_error(void) {
	fprintf(stderr, "Error number: %d %s\n", errno, strerror(errno));
	exit(EXIT_FAIL);
}

void commandline_error(void) {
	fprintf(stderr, "Correct Usage: ./lab2_add --threads=# --iterations=# [--yield] [--sync=msc]\n");
	exit(EXIT_FAIL);
}

void* modify_list_none(SortedListElement_t *element) {
	for (long long i = 0; i != iterations; i++) {
		SortedList_insert(head, element + i);
	}

	int length = SortedList_length(head);
	if (length == -1) {
		fprintf(stderr, "SortedList_length failed: List was corrupted\n");
		exit(EXIT_OTHER);
	}

	for (long long i = 0; i != iterations; i++) {
		SortedListElement_t *del_element = SortedList_lookup(head, (element + i)->key);
		if (del_element == NULL) {
			fprintf(stderr, "SortedList_lookup failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		if (SortedList_delete(del_element) != 0) {
			fprintf(stderr, "SortedList_delete failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
	}
	return NULL;
}

void* modify_list_spin(SortedListElement_t *element) {
	for (long long i = 0; i != iterations; i++) {
		while (__sync_lock_test_and_set(&spin_lock, 1) == 1)
			;
		SortedList_insert(head, element + i);
		__sync_lock_release(&spin_lock);
	}

	while (__sync_lock_test_and_set(&spin_lock, 1) == 1)
		;	
	int length = SortedList_length(head);
	__sync_lock_release(&spin_lock);

	if (length == -1) {
		fprintf(stderr, "SortedList_length failed: List was corrupted\n");
		exit(EXIT_OTHER);
	}

	for (long long i = 0; i != iterations; i++) {
		while (__sync_lock_test_and_set(&spin_lock, 1) == 1)
			;		
		SortedListElement_t *del_element = SortedList_lookup(head, (element + i)->key);
		if (del_element == NULL) {
			fprintf(stderr, "SortedList_lookup failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		if (SortedList_delete(del_element) != 0) {
			fprintf(stderr, "SortedList_delete failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		__sync_lock_release(&spin_lock);
	}
	return NULL;
}

void* modify_list_mutex(SortedListElement_t *element) {
	for (long long i = 0; i != iterations; i++) {
		if (pthread_mutex_lock(&lock) != 0) {
			syscall_error();
		}
		SortedList_insert(head, element + i);
	    if (pthread_mutex_unlock(&lock) != 0) {
    		syscall_error();
    	}	
	}

    if (pthread_mutex_lock(&lock) != 0) {
    	syscall_error();
    }
	int length = SortedList_length(head);
    if (pthread_mutex_unlock(&lock) != 0) {
    	syscall_error();
    }

	if (length == -1) {
		fprintf(stderr, "SortedList_length failed: List was corrupted\n");
		exit(EXIT_OTHER);
	}

	for (long long i = 0; i != iterations; i++) {
	    if (pthread_mutex_lock(&lock) != 0) {
    		syscall_error();
    	}	
		SortedListElement_t *del_element = SortedList_lookup(head, (element + i)->key);
		if (del_element == NULL) {
			fprintf(stderr, "SortedList_lookup failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		if (SortedList_delete(del_element) != 0) {
			fprintf(stderr, "SortedList_delete failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		if (pthread_mutex_unlock(&lock) != 0) {
    		syscall_error();
    	}
	}
	return NULL;
}

void *thread_func(void *pointer) {
	SortedListElement_t *element = pointer;
	if (!sync_flag)
		modify_list_none(element);
	else {
		if (sync_opt == 's')
			modify_list_spin(element);
		else if (sync_opt == 'm')
			modify_list_mutex(element);
	}
	pthread_exit(NULL);
}

void randString(int length, char *ret_string) {
	
	if (ret_string == NULL) {
		syscall_error();
	}
	for (int i = 0; i != length; i++) {
		ret_string[i] = (char) ('a' + (rand() % 26));
	}
	ret_string[length] = '\0';
}

int main(int argc, char **argv) {
	srand(time(NULL));
	int opt_ret;

	struct option longopts[] = {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
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
				for (int i = 0; i != (int) strlen(optarg); i++) {
					if (optarg[i] == 'i')
						opt_yield = opt_yield | INSERT_YIELD;
					else if (optarg[i] == 'd')
						opt_yield = opt_yield | DELETE_YIELD;
					else if (optarg[i] == 'l')
						opt_yield = opt_yield | LOOKUP_YIELD;
					else
						commandline_error();
				}
				break;

			case 's':
				if (strlen(optarg) != 1) {
					commandline_error();
				}
				sync_flag = 1;
				sync_opt = optarg[0];
				if (sync_opt != 'm' && sync_opt != 's') {
					commandline_error();
				}
				break;

			default:
				commandline_error();
				break;

		}
	}

	signal(SIGSEGV, handler);

	// initialize list
	if ((head = malloc(sizeof(SortedList_t))) == NULL) {
		syscall_error();
	}
	head->next = head;
	head->prev = head;
	head->key = NULL;

	// array of list elements
	SortedListElement_t *elementArr;
	if ((elementArr = (SortedListElement_t *) malloc(numthread * iterations * sizeof(SortedListElement_t))) == NULL) {
		syscall_error();
	}

	for (long long i = 0; i != numthread * iterations; i++) {
		int length = 1 + (rand() % 20);
		char *ret_string = malloc(sizeof(char) * (length + 1));
		randString(length, ret_string); 
		elementArr[i].key = ret_string;
	}


	pthread_t threads[numthread];

	// get the starting time for the run
	struct timespec begin, end;
	if (clock_gettime(CLOCK_REALTIME, &begin) < -1) {
		syscall_error();
	}

	for (long long i = 0; i != numthread; i++) {
		if (pthread_create(&threads[i], NULL, thread_func, elementArr + (i * iterations))) {
			syscall_error(); 
		}
	}

	for (long long i = 0; i != numthread; i++) {
		if (pthread_join(threads[i], NULL)) {
			syscall_error();
		}
	}

	if (clock_gettime(CLOCK_REALTIME, &end) < -1) {
		syscall_error();
	}

	if (SortedList_length(head) != 0) {
		fprintf(stderr, "Length of list after all insertions and deletions is not 0\n");
		exit(EXIT_OTHER);
	}	

	// get ending time for run
	long long elasped_ns = end.tv_nsec - begin.tv_nsec;
	long long total_ops = numthread*iterations*3;



	char* yield_type;
	if ((opt_yield & INSERT_YIELD) && (opt_yield & DELETE_YIELD) && (opt_yield & LOOKUP_YIELD))
		yield_type = "idl";
	else if ((opt_yield & INSERT_YIELD) && (opt_yield & DELETE_YIELD))
		yield_type = "id";
	else if ((opt_yield & DELETE_YIELD) && (opt_yield & LOOKUP_YIELD))
		yield_type = "dl";
	else if ((opt_yield & INSERT_YIELD) && (opt_yield & LOOKUP_YIELD))
		yield_type = "il";
	else if ((opt_yield & INSERT_YIELD))
		yield_type = "i";
	else if ((opt_yield & DELETE_YIELD))
		yield_type = "d";
	else if ((opt_yield & LOOKUP_YIELD))
		yield_type = "l";
	else
		yield_type = "none";

	char* sync_type;
	if (!sync_flag)
		sync_type = "none";
	else if (sync_opt == 'm')
		sync_type = "m";
	else if (sync_opt == 's')
		sync_type = "s";


	printf("list-%s-%s,%lld,%lld,%d,%lld,%lld,%lld\n", 
		yield_type, sync_type, numthread, iterations, 1, 
		total_ops, elasped_ns, elasped_ns/total_ops);

	for (long long i = 0; i != numthread * iterations; i++) {
		free((void *) (elementArr[i].key));
	}

	free(elementArr);
	free(head);
	exit(EXIT_SUCCESS);
}