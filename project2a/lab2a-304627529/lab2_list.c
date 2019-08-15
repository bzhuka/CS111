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
int opt_yield = 0;
char opt_yield2[5];
int opt_syncs = 0;
int opt_syncm = 0;

SortedListElement_t head;

pthread_mutex_t tmutex;
int spin = 0;

void* fThreads(void* param) {
	int i;
	SortedListElement_t *list = (SortedListElement_t *)param;

	if (opt_syncm == 1) {
		//Inserts into a global list
		for (i = 0; i < nIterations; i++) {
			pthread_mutex_lock(&tmutex);
			SortedList_insert(&head, list + i);
			pthread_mutex_unlock(&tmutex);
		}

		//Gets length of list
		pthread_mutex_lock(&tmutex);
		//printf("Length of list: %d\n", SortedList_length(&head));
		SortedList_length(&head);
		pthread_mutex_unlock(&tmutex);

		//Look up and deletes each key it has inserted
		for (i = 0; i < nIterations; i++) {
			pthread_mutex_lock(&tmutex);
			SortedListElement_t *target;
			target = SortedList_lookup(&head, (list + i)->key);
			if (target == NULL) {
				fprintf(stderr, "List corrupted.\n");
				exit(-1);
			}
			SortedList_delete(target);
			pthread_mutex_unlock(&tmutex);
		}
	} else if (opt_syncs == 1) {
		//Inserts into a golbal list
		for (i = 0; i < nIterations; i++) {
			while(__sync_lock_test_and_set(&spin, 1));
			SortedList_insert(&head, list + i);
			__sync_lock_release(&spin);
		}

		//Gets length of list
		while(__sync_lock_test_and_set(&spin, 1));
		//printf("Length of list: %d\n", SortedList_length(&head));
		SortedList_length(&head);
		__sync_lock_release(&spin);

		//Looks up and deletes each key it has inserted
		for (i = 0; i < nIterations; i++) {
			while(__sync_lock_test_and_set(&spin, 1));
			SortedListElement_t *target;
			target = SortedList_lookup(&head, (list + i)->key);
			if (target == NULL) {
				fprintf(stderr, "List corrupted.\n");
				exit(-1);
			}
			SortedList_delete(target);
			__sync_lock_release(&spin);
		}
	} else {
		//Inserts into a global list
		for (i = 0; i < nIterations; i++) {
			SortedList_insert(&head, list + i);
		}

		//Gets length of list
		//printf("Length of list: %d\n", SortedList_length(&head));
		SortedList_length(&head);

		//Look up and deletes each of the keys it has inserted
		for (i = 0; i < nIterations; i++) {
			SortedListElement_t *target;
			target = SortedList_lookup(&head, (list + i)->key);
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
		}
	}

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

	if (opt_syncm == 1) {
		pthread_mutex_init(&tmutex, NULL);
	}

	//initializes an empty list
	head.next = NULL; 
	head.key = NULL;
	head.prev = NULL;

	//Creates and initializes with random keys the list elements
	int nElements = nThreads * nIterations;
	SortedListElement_t list[nElements];
	char elements[nElements];
	for (i = 0; i < nElements; i++) {
		elements[i] = rand() % 128; 
		list[i].key = &elements[i];
	}

	//Notes the (high resoultion) starting time
	struct timespec start;
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//Adds the Specified number of threads
	pthread_t threads[nThreads];
	int rc;
	for (i = 0; i < nThreads; i++) {
		rc = pthread_create(&threads[i], NULL, fThreads, &(list[i * nIterations]));
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
		}
	}

	//Notes the (high resolution) ending time
	int temp = clock_gettime(CLOCK_MONOTONIC, &end);
	if (temp != 0) {
		fprintf(stderr, "Length at end was: %d\n", temp);
	}

	//Checks the length of the list
	//printf("Final length of list: %d\n", SortedList_length(&head));	
	SortedList_length(&head);

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
	i = sprintf(message, "1,");
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
	i = sprintf(message, "%lld\n", avgrtime);
	write(1, message, i);


	return 0;
}
