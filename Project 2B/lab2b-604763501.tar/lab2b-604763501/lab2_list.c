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
long long *wait;

int num_list = 1;
typedef struct sublist {
	SortedList_t *list;
	int spin_lock;
	pthread_mutex_t lock;
} sublist;
sublist *sublist_arr;				// array of pointers to lists
int *hash_key;
SortedListElement_t *elementArr;

// signal handler for SIGSEGV
void handler(int sig) {
	if (sig == SIGSEGV) {
		printf("segfault\n");
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

int hash(const char* key) {
	return key[0] % num_list;
}

void* modify_list_none(long long index) {
	long long totalElements = iterations * numthread;
	for (long long i = index; i < totalElements; i+=numthread) {
		SortedList_insert((sublist_arr + hash_key[i])->list, elementArr + i);
	}

	for (int i = 0; i != num_list; i++) {
		int length = SortedList_length((sublist_arr + i)->list);
		if (length == -1) {
			fprintf(stderr, "SortedList_length failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
	}

	for (long long i = index; i < totalElements; i+=numthread) {
		SortedListElement_t *del_element = SortedList_lookup((sublist_arr + hash_key[i])->list, (elementArr + i)->key);
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

void* modify_list_spin(long long index) {
	long long totalElements = iterations * numthread;
	for (long long i = index; i < totalElements; i+=numthread) {
		int key = hash_key[i];
		struct timespec begin, end;
		if (clock_gettime(CLOCK_MONOTONIC, &begin) < 0) {
			syscall_error();
		}

		while (__sync_lock_test_and_set(&((sublist_arr + key)->spin_lock), 1))
			;	

		if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
			syscall_error();
		}

		SortedList_insert((sublist_arr + key)->list, elementArr + i);
		__sync_lock_release(&((sublist_arr + key)->spin_lock));	

		wait[key] += (1000000000L * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec));
	}

	for (int i = 0; i != num_list; i++) {
		struct timespec begin, end;
		if (clock_gettime(CLOCK_MONOTONIC, &begin) < 0) {
			syscall_error();
		}

		while (__sync_lock_test_and_set(&((sublist_arr + i)->spin_lock), 1))
			;

		if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
			syscall_error();
		}


		int length = SortedList_length((sublist_arr + i)->list);
		__sync_lock_release(&((sublist_arr + i)->spin_lock));

		if (length == -1) {
			fprintf(stderr, "SortedList_length failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		wait[i] += (1000000000L * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec));
	}

	for (long long i = index; i < totalElements; i+=numthread) {
		int key = hash_key[i];
		struct timespec begin, end;
		if (clock_gettime(CLOCK_MONOTONIC, &begin) < 0) {
			syscall_error();
		}

		while (__sync_lock_test_and_set(&((sublist_arr + key)->spin_lock), 1))
			;

		if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
			syscall_error();
		}


		SortedListElement_t *del_element = SortedList_lookup((sublist_arr + key)->list, (elementArr + i)->key);
		if (del_element == NULL) {
			fprintf(stderr, "SortedList_lookup failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		if (SortedList_delete(del_element) != 0) {
			fprintf(stderr, "SortedList_delete failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		__sync_lock_release(&((sublist_arr + key)->spin_lock));
		wait[key] += (1000000000L * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec));
	}
	return NULL;
}

void* modify_list_mutex(long long index) {
	long long totalElements = iterations * numthread;
	for (long long i = index; i < totalElements; i+=numthread) {
		int key = hash_key[i];
		struct timespec begin, end;
		if (clock_gettime(CLOCK_MONOTONIC, &begin) < 0) {
			syscall_error();
		}

		if (pthread_mutex_lock(&((sublist_arr + key)->lock)) != 0) {
			syscall_error();
		}

		if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
			syscall_error();
		}

		SortedList_insert((sublist_arr + key)->list, elementArr + i);
	    if (pthread_mutex_unlock(&((sublist_arr + key)->lock)) != 0) {
    		syscall_error();
    	}
		wait[key] += (1000000000L * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec));	
	}

	for (int i = 0; i != num_list; i++) {
		struct timespec begin, end;
		if (clock_gettime(CLOCK_MONOTONIC, &begin) < 0) {
			syscall_error();
		}

	    if (pthread_mutex_lock(&((sublist_arr + i)->lock)) != 0) {
	    	syscall_error();
	    }

		if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
			syscall_error();
		}

		int length = SortedList_length((sublist_arr + i)->list);
	    if (pthread_mutex_unlock(&((sublist_arr + i)->lock)) != 0) {
	    	syscall_error();
	    }

		if (length == -1) {
			fprintf(stderr, "SortedList_length failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}

		wait[i] += (1000000000L * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec));
	}

	for (long long i = index; i < totalElements; i+=numthread) {
		int key = hash_key[i];
		struct timespec begin, end;
		if (clock_gettime(CLOCK_MONOTONIC, &begin) < 0) {
			syscall_error();
		}

	    if (pthread_mutex_lock(&((sublist_arr + key)->lock)) != 0) {
    		syscall_error();
    	}	

		if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
			syscall_error();
		}

		SortedListElement_t *del_element = SortedList_lookup((sublist_arr + key)->list, (elementArr + i)->key);
		if (del_element == NULL) {
			fprintf(stderr, "SortedList_lookup failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		if (SortedList_delete(del_element) != 0) {
			fprintf(stderr, "SortedList_delete failed: List was corrupted\n");
			exit(EXIT_OTHER);
		}
		if (pthread_mutex_unlock(&((sublist_arr + key)->lock)) != 0) {
    		syscall_error();
    	}
		wait[key] += (1000000000L * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec));
	}
	return NULL;
}

void *thread_func(void *pointer) {
	long long* threadNum = pointer;
	if (!sync_flag)
		modify_list_none(*threadNum);
	else {
		if (sync_opt == 's')
			modify_list_spin(*threadNum);
		else if (sync_opt == 'm')
			modify_list_mutex(*threadNum);
	}
	pthread_exit(NULL);
}

/* void randString(int length, char *ret_string) {
	
	if (ret_string == NULL) {
		syscall_error();
	}
	for (int i = 0; i != length; i++) {
		ret_string[i] = (char) ('a' + (rand() % 26));
	}
	ret_string[length] = '\0';
} */

void randString(char *ret_string) {
	
	if (ret_string == NULL) {
		syscall_error();
	}
	ret_string[0] = (char) ('a' + (rand() % 26));
	ret_string[1] = '\0';
}

int main(int argc, char **argv) {
	srand(time(NULL));
	int opt_ret;

	struct option longopts[] = {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{"lists", required_argument, NULL, 'l'},
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

			case 'l':
				num_list = atoi(optarg);
				break;	

			default:
				commandline_error();
				break;

		}
	}

	signal(SIGSEGV, handler);

	// initialize list
	if ((sublist_arr = malloc(sizeof(sublist) * num_list)) == NULL) {
		syscall_error();
	}

	if ((wait = malloc(sizeof(long long) * num_list)) == NULL) {
		syscall_error();
	}

	for (int i = 0; i != num_list; i++) {
		sublist* s = sublist_arr + i;

		s->list = malloc(sizeof(SortedList_t));
		if (s->list == NULL) {
			syscall_error();
		}
		s->spin_lock = 0;
		pthread_mutex_init(&(s->lock), NULL);
		s->list->next = s->list;
		s->list->prev = s->list;
		s->list->key = NULL;
		wait[i] = 0;
	}

	// array of list elements
	if ((elementArr = (SortedListElement_t *) malloc(numthread * iterations * sizeof(SortedListElement_t))) == NULL) {
		syscall_error();
	}

	// array to hold the hash key for all element keys
	if ((hash_key = malloc(numthread * iterations * sizeof(int))) == NULL) {
		syscall_error();
	}

	for (long long i = 0; i != numthread * iterations; i++) {
		// int length = 1 + (rand() % 20);
		// char *ret_string = malloc(sizeof(char) * (length + 1));
		// randString(length, ret_string);
		char *ret_string = malloc(sizeof(char) * 2);
		randString(ret_string); 
		elementArr[i].key = ret_string;
		hash_key[i] = hash(elementArr[i].key);
	}


	// thread setup
	pthread_t threads[numthread];
	long long *threadIDs;
	if ((threadIDs = malloc(numthread * sizeof(long long))) == NULL) {
		syscall_error();
	}



	// get the starting time for the run
	struct timespec begin, end;
	if (clock_gettime(CLOCK_MONOTONIC, &begin) < 0) {
		syscall_error();
	}

	for (long long i = 0; i != numthread; i++) {
		threadIDs[i] = i;
		if (pthread_create(&threads[i], NULL, thread_func, threadIDs + i)) {
			syscall_error(); 
		}
	}

	for (long long i = 0; i != numthread; i++) {
		if (pthread_join(threads[i], NULL)) {
			syscall_error();
		}
	}

	if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
		syscall_error();
	}



	long long final_list_length = 0;
	for (int i = 0; i != num_list; i++) {
		final_list_length += SortedList_length((sublist_arr + i)->list);
	}
	if (final_list_length != 0) {
		fprintf(stderr, "Length of list after all insertions and deletions is not 0\n");
		exit(EXIT_OTHER);
	}	

	// get ending time for run
	long long elasped_ns = (1000000000L * (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec));
	// long long elasped_ns = end.tv_nsec - begin.tv_nsec;
	long long total_ops = numthread*iterations*3;
	long long lock_ops = numthread*(iterations*3 + 1);
	long long total_wait = 0;
	for (int i  = 0; i != num_list; i++) {
		total_wait += wait[i];
	}

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


	printf("list-%s-%s,%lld,%lld,%d,%lld,%lld,%lld,%lld\n", 
		yield_type, sync_type, numthread, iterations, num_list, 
		total_ops, elasped_ns, elasped_ns/total_ops, total_wait/lock_ops);

	for (long long i = 0; i != numthread * iterations; i++) {
		free((void *) (elementArr[i].key));
	}

	free(elementArr);
	for (int i = 0; i != num_list; i++) {
		free((sublist_arr + i)->list);
	}
	free(sublist_arr);
	free(wait);
	exit(EXIT_SUCCESS);
}