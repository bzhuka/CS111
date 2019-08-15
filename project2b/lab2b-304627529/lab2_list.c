#include "SortedList.h"

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUFFERSIZE 512

int nThreads = 1;
int nIterations = 1;
int nLists = 1;
int opt_yield = 0;
char opt_yield2[5];
int opt_syncs = 0;
int opt_syncm = 0;
long long *lock_time;
int* offset;

SortedListElement_t *head;
SortedListElement_t *list;

pthread_mutex_t* tmutex;
int* spin;

int hash(const char *key) {
	int temp = *key;
	return temp%nLists;
}

void* fThreads(void* param) {
	int i;
	int thread_id = *(int*)param;

	if (opt_syncm == 1) {//If mutex
		struct timespec mstart;
		struct timespec mend;
		long long mtime = 0;

		//Inserts into a global list
		for (i = thread_id; i < nIterations * nThreads; i+=nThreads) {
			clock_gettime(CLOCK_MONOTONIC, &mstart);
			pthread_mutex_lock(&tmutex[offset[i]]);
			clock_gettime(CLOCK_MONOTONIC, &mend);
			SortedList_insert(&head[offset[i]], &list[i]);
			pthread_mutex_unlock(&tmutex[offset[i]]);
			long long rtime = (mend.tv_sec - mstart.tv_sec) * 1000000000;
			rtime = rtime + mend.tv_nsec - mstart.tv_nsec;
			mtime += rtime;

		}

		//Gets length of list
		for (i = 0; i < nLists; i++) {
			clock_gettime(CLOCK_MONOTONIC, &mstart);
			pthread_mutex_lock(&tmutex[i]);
			clock_gettime(CLOCK_MONOTONIC, &mend);
			SortedList_length(&head[i]);
			pthread_mutex_unlock(&tmutex[i]);
			long long rtime = (mend.tv_sec - mstart.tv_sec) * 1000000000;
			rtime = rtime + mend.tv_nsec - mstart.tv_nsec;
			mtime += rtime;
		}

		//Look up and deletes each key it has inserted
		for (i = thread_id; i < nIterations * nThreads; i+=nThreads) {
			clock_gettime(CLOCK_MONOTONIC, &mstart);
			pthread_mutex_lock(&tmutex[offset[i]]);
			clock_gettime(CLOCK_MONOTONIC, &mend);
			SortedListElement_t *target;
			target = SortedList_lookup(&head[offset[i]], list[i].key);
			if (target == NULL) {
				fprintf(stderr, "List corrupted.\n");
				exit(-1);
			}
			SortedList_delete(target);
			pthread_mutex_unlock(&tmutex[offset[i]]);
			long long rtime = (mend.tv_sec - mstart.tv_sec) * 1000000000;
			rtime = rtime + mend.tv_nsec - mstart.tv_nsec;
			mtime += rtime;
		}
		lock_time[thread_id] = mtime;
	} else if (opt_syncs == 1) {//Spin lock
		//Inserts into a global list
		for (i = thread_id; i < nIterations * nThreads; i+=nThreads) {
			while(__sync_lock_test_and_set(&spin[offset[i]], 1));
			SortedList_insert(&head[offset[i]], &list[i]);
			__sync_lock_release(&spin[offset[i]]);
		}

		//Gets length of list
		for (i = 0; i < nLists; i++) {
			while(__sync_lock_test_and_set(&spin[i], 1));
			SortedList_length(&head[i]);
			__sync_lock_release(&spin[i]);
		}

		//Looks up and deletes each key it has inserted
		for (i = thread_id; i < nIterations * nThreads; i+=nThreads) {
			while(__sync_lock_test_and_set(&spin[offset[i]], 1));
			SortedListElement_t *target;
			target = SortedList_lookup(&head[offset[i]], list[i].key);
			if (target == NULL) {
				fprintf(stderr, "List corrupted.\n");
				exit(-1);
			}
			SortedList_delete(target);
			__sync_lock_release(&spin[offset[i]]);
		}
	} else {
		//Inserts into a global list
		for (i = thread_id; i < nIterations * nThreads; i+=nThreads) {
			SortedList_insert(&head[offset[i]], &list[i]);
		}

		//Gets length of list
		for (i = 0; i < nLists; i++) {
			SortedList_length(&head[i]);
		}

		//Look up and deletes each of the keys it has inserted
		for (i = thread_id; i < nIterations * nThreads; i+=nThreads) {
			SortedListElement_t *target;
			target = SortedList_lookup(&head[offset[i]], list[i].key);
			if (target == NULL) {
				fprintf(stderr, "List corrupted.\n");
				exit(-1);
			}
			SortedList_delete(target);
		}
	}
	return NULL;
}

int main(int argc, char*argv[]) {
	int i;

	//Get options
	static struct option long_options[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{"lists", required_argument, 0, 'l'},
		{0,0,0,0}
	};

	int j = 0;
	int k;
	char s_opt;
	while(1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "t:i:y:", long_options, &option_index);
		if (c == -1)
			break;

		switch(c) {
			case 't':
				nThreads = atoi(optarg);
				break;
			case 'i':
				nIterations = atoi(optarg);
				break;
			case 'y':
				for (i = 0; i < strlen(optarg); i++) {
					if (optarg[i] == 'i') {
						opt_yield |= INSERT_YIELD;
						j += 1;
					} else if (optarg[i] == 'd') {
						opt_yield |= DELETE_YIELD;
						j += 2;
					} else if (optarg[i] == 'l') {
						opt_yield |= LOOKUP_YIELD;
						j += 4;
					}
				}
				break;
			case 's':
				s_opt = optarg[0];
				if (s_opt == 'm')
					opt_syncm = 1;
				else if (s_opt == 's')
					opt_syncs = 1;
				else {
					printf("Incorrect option for sync.\n");
					exit(-1);
				}
				break;
			case 'l':
				nLists = atoi(optarg);
				break;
		}
	}

	//Figuring out print statement later
	switch(j) {
		case 0:
			k = sprintf(opt_yield2, "none");
			break;
		case 1:
			k = sprintf(opt_yield2, "i");
			break;
		case 2:
			k = sprintf(opt_yield2, "d");
			break;
		case 3:
			k = sprintf(opt_yield2, "id");
			break;
		case 4: 
			k = sprintf(opt_yield2, "l");
			break;
		case 5:
			k = sprintf(opt_yield2, "il");
			break;
		case 6: 
			k = sprintf(opt_yield2, "dl");
			break;
		case 7:
			k = sprintf(opt_yield2, "idl");
			break;
	}

	//For mutex lock initialize pthread_mutex_t, for spin lock initialize spins
	if (opt_syncm == 1) {
		tmutex = malloc(sizeof(pthread_mutex_t) * nLists);
		for (i = 0; i < nLists; i++) {
			pthread_mutex_init(&tmutex[i], NULL);
		}
		lock_time = (long long *)malloc(sizeof(long long) * nThreads);
	} else if (opt_syncs == 1) {
		spin = malloc(sizeof(int) * nLists);
		for (i = 0; i < nLists; i++) {
			spin[i] = 0;
		}
	}

	//initializes an empty list
	head = (SortedListElement_t *)malloc(sizeof(SortedListElement_t) * nLists);
	for (i = 0; i < nLists; i++) {
		head[i].next = NULL; 
		head[i].key = NULL;
		head[i].prev = NULL;
	}

	//Creates and initializes with random keys the list elements
	int nElements = nThreads * nIterations;
	list = (SortedListElement_t *)malloc(sizeof(SortedListElement_t) * nElements);
	char elements[nElements];
	for (i = 0; i < nElements; i++) {
		elements[i] = rand() % 128; 
		list[i].key = &elements[i];
	}

	//thread id's
	pthread_t threads[nThreads];
	int* ids = malloc(nThreads * sizeof(int));	

	//calculates offset
	offset = malloc(sizeof(int) * nElements);
	for(i = 0; i < nElements; i++) {
		offset[i] = hash(list[i].key);
	}

	//Notes the (high resoultion) starting time
	struct timespec start;
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//adds all threads
	int rc;
	for (i = 0; i < nThreads; i++) {
		ids[i] = i;
		rc = pthread_create(&threads[i], NULL, fThreads, &ids[i]);
		if (rc) {
			printf("Error in pthread_create: %d\n", rc);
			exit(-1);
		}
	}

	//Waits for all threads to complete
	for (i = 0; i < nThreads; i++) {
		rc = pthread_join(threads[i], NULL);
		if (rc) {
			printf("Error in pthread_join: %d\n", rc);
			exit(-1);
		}
	}

	//Notes the (high resolution) ending time
	int temp = clock_gettime(CLOCK_MONOTONIC, &end);
	if (temp != 0) {
		fprintf(stderr, "Error getting time: %d\n", temp);
		exit(-1);
	}

	//Checks the length of the list
	int errtest = 0;
	for (i = 0; i < nLists; i++) {
		errtest += SortedList_length(&head[i]);
	}
	if (errtest != 0) {
		fprintf(stderr, "Corrupted lists, final count: %d\n", errtest);
		exit(-1);
	}

	//Print out values
	char message[BUFFERSIZE] = {};

	//Test name
	i = sprintf(message, "list-");
	write(1, message, i);
	write(1, opt_yield2, k);
	if (opt_syncm == 1) {
		i = sprintf(message, "-m,");
	} else if (opt_syncs == 1) {
		i = sprintf(message, "-s,");
	} else {
		i = sprintf(message, "-none,");
	}
	write(1, message, i);

	//Number of threads
	i = sprintf(message, "%d,", nThreads);
	write(1, message, i);

	//Number of iterations
	i = sprintf(message, "%d,", nIterations);
	write(1, message, i);

	//Number of lists
	i = sprintf(message, "%d,", nLists);
	write(1, message, i);

	//Number of operations
	int ops = nThreads * nIterations * 3;
	i = sprintf(message, "%d,", ops);
	write(1, message, i);

	//Total run time
	long long rtime = (end.tv_sec - start.tv_sec) * 1000000000;
	rtime = rtime + end.tv_nsec - start.tv_nsec;
	i = sprintf(message, "%lld,", rtime);
	write(1, message, i);

	//Average run time
	long long avgrtime = rtime / ops;
	i = sprintf(message, "%lld,", avgrtime);
	write(1, message, i);

	//If mutex lock, finds average wait-for-lock
	if (opt_syncm == 1) {
		long long totaltime = 0;
		for(i = 0; i < nThreads; i++) {
			totaltime += lock_time[i];
		}
		totaltime /= ((2 * nIterations + 1)*nThreads);
		i = sprintf(message, "%lld\n", totaltime);
		write(1, message, i);
	} else {
		i = sprintf(message, "0\n");
		write(1, message, i);
	}

	free(head);
	free(list);
	free(ids);
	free(lock_time);


	return 0;
}
