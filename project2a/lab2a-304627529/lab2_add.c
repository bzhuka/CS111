#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BUFFERSIZE 512

int opt_syncm;
int opt_syncs;
int opt_syncc;
int opt_yield;

int nThreads = 1;
int nIterations = 1;

int spin = 0;

long long counter = 0;

pthread_mutex_t tmutex;

void add(long long *pointer, long long value) {
	long long sum = *pointer + value;
	if (opt_yield)
		sched_yield();
	*pointer = sum;
}

void *fThreads() {
	int i;
	long long prev = 0;

	if (opt_syncm == 1) {
		for (i = 0; i < nIterations; i++) {
			pthread_mutex_lock(&tmutex);
			add(&counter, 1);
			pthread_mutex_unlock(&tmutex);
		}
		for (i = 0; i < nIterations; i++) {
			pthread_mutex_lock(&tmutex);
			add(&counter, -1);
			pthread_mutex_unlock(&tmutex);
		}
	} else if (opt_syncs == 1) {
		for (int i = 0; i < nIterations; i++) {		
			while(__sync_lock_test_and_set(&spin, 1));
			add(&counter, 1);
			__sync_lock_release(&spin);
		}
		for (int i = 0; i < nIterations; i++) {
			while(__sync_lock_test_and_set(&spin, 1));
			add(&counter, -1);
			__sync_lock_release(&spin);
		}
	} else if (opt_syncc == 1) {
		for (int i = 0; i < nIterations; i++) {
			do {
				prev = counter;
				if (opt_yield)
					sched_yield();
			} while (__sync_val_compare_and_swap(&counter, prev, prev+1) != prev);
		}
                for (int i = 0; i < nIterations; i++) {
                        do {
                                prev = counter;
                                if (opt_yield) 
                                        sched_yield();
                        } while (__sync_val_compare_and_swap(&counter, prev, prev-1) != prev);
                }
	} else {
		for(i = 0; i < nIterations; i++) {
			add(&counter, 1);
		}
		for(i = 0; i < nIterations; i++) {
			add(&counter, -1);
		}
	}

	return NULL;
}

int main(int argc, char*argv[]) {
	//Gets options
	static struct option long_options[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", no_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0,0,0,0}
	};
	char s_opt;
	while(1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "t:i:ys:", long_options, &option_index);
		if(c == -1)
			break;

		switch(c) {
			case 't':
				nThreads = atoi(optarg);
				break;
			case 'i':
				nIterations = atoi(optarg);
				break;
			case 'y':
				opt_yield = 1;
				break;
			case 's':
				s_opt = optarg[0];
				if (s_opt == 'm')
					opt_syncm = 1;
				else if (s_opt == 's')
					opt_syncs = 1;
				else if (s_opt == 'c')
					opt_syncc = 1;
				else {
					printf("Incorrect option for sync.\n");
					exit(-1);
				}
				break;
		}
	}

	//If sync mutex, then initialize it
	if (opt_syncm == 1) {
		pthread_mutex_init(&tmutex, NULL);
	}

	//Notes the (high resolution) starting time for the run
	struct timespec start;
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	//Adds the specified number of threads
	pthread_t threads[nThreads];
	int rc;
	int i;
	for (i = 0; i < nThreads; i++) {
		rc = pthread_create(&threads[i], NULL, fThreads, NULL);
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
	clock_gettime(CLOCK_MONOTONIC, &end);

	//Print out values
	char message[BUFFERSIZE] = {};

	//Test name
	i = sprintf(message, "add");
	write(1, message, i);
	if (opt_yield == 1) {
		i = sprintf(message, "-yield");
		write(1, message, i);
	}
	if (opt_syncm == 1) {
		i = sprintf(message, "-m,");
	} else if (opt_syncs == 1) {
		i = sprintf(message, "-s,");
	} else if (opt_syncc == 1) {
		i = sprintf(message, "-c,");
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

	//Number of operations performed
	int ops = nThreads * nIterations * 2;
	i = sprintf(message, "%d,", ops);
	write(1, message, i);

	//Total run time in nanoseconds
	long long rtime = (end.tv_sec - start.tv_sec) * 1000000000;
	rtime = rtime + end.tv_nsec - start.tv_nsec;
	i = sprintf(message, "%lld,", rtime);
	write(1, message, i);

	//Average run time
	long long avgrtime = rtime / ops;
	i = sprintf(message, "%lld,", avgrtime);
	write(1, message, i);

	//Total at the end of the run
	i = sprintf(message, "%lld\n", counter);
	write(1, message, i);

	return 0;
}
