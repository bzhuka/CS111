#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <mcrypt.h>
#include <fcntl.h>

#define BUFFERSIZE 256
#define PORTNO 121345

int encrypt = 0;
pid_t child_pid;
pthread_t pthread_id;
pthread_mutex_t p_lock;
MCRYPT td;

void my_exit(int status) {
	//pthread_mutex_lock(&p_lock);
	//kill(child_pid, SIGINT);
	exit(status);
}

static void sigpipe_handler(int sig) {
	my_exit(2);
}

void *read_from_child(void *arg) {
	int *pipe = (int*)arg;
	char buffer[BUFFERSIZE] = {};
	while(1) {
		int i = read(*pipe, buffer, BUFFERSIZE);

		if (i > 0) {
			if (encrypt == 1) {
				mcrypt_generic(td, buffer, i);
			}
			int j = write(1, buffer, i);
			if (j < 1) {
				my_exit(1);
			}
		} else {
			my_exit(2);
		}
	}
}

int main( int argc, char *argv[] ) {
	int sockfd, newsockfd, portno;
	unsigned int clilen;
	char buffer[1];
	struct sockaddr_in serv_addr, cli_addr;
	portno = PORTNO;

	static struct option long_options[] = {
		{"port", required_argument, 0, 'p'},
		{"encrypt", no_argument, 0, 'e'},
		{0, 0, 0, 0}
	};

	while(1) {
		int option_index = 0;

		int c = getopt_long(argc, argv, "p:e", long_options, &option_index);
		if (c == -1)
			break;

		switch(c) {
			case'p':
				portno = atoi(optarg);
				break;
			case 'e':
				encrypt = 1;
				break;
		}
	}

	/* First call to socket() function */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	signal(SIGPIPE, sigpipe_handler);

	/* Initialize socket structure */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	/* Now bind the host address using bind() call.*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	if (encrypt == 1) {
		int i;
		char *key;
		char password[20];
		char *IV;
		int keysize = 16; /* 128 bytes */
		key = calloc(1, keysize);
		int key_fd = open("my.key", O_RDONLY);
		if(key_fd < 0) {
			perror("Key error.\n");
			my_exit(3);
		}
		int rsize = read(key_fd, password, 20);
		password[rsize] = 0;
		memmove( key, password, strlen(password));
		td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
		if (td==MCRYPT_FAILED) {
			perror("Mcrypt failure.\n");
			my_exit(3);
		}
		IV = malloc(mcrypt_enc_get_iv_size(td));
		/* Put random data in IV. Note these are not real random data,
		 *                  *  * consider using /dev/random or /dev/urandom.
		 *                                   *   */
		/*  srand(time(0)); */
		for (i=0; i< mcrypt_enc_get_iv_size( td); i++) {
			IV[i]=i+1;
		}
		i=mcrypt_generic_init( td, key, keysize, IV);
		if (i<0) {
			mcrypt_perror(i);
			my_exit(3);
		}
	}

	pthread_mutex_init(&p_lock, NULL);

	/* Now start listening for the clients, here process will
	 *       * go in sleep mode and will wait for the incoming connection
	 *          */

	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	while(1) {
		/* Accept actual connection from the client */
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

		if (newsockfd < 0) {
			perror("ERROR on accept");
			exit(1);
		}

		int to_child_pipe[2];
		int from_child_pipe[2];
		pid_t child_pid = -1;

		if (pipe(to_child_pipe) == -1) {
			fprintf(stderr, "pipe() failed.\n");
			my_exit(1);
		}

		if(pipe(from_child_pipe) == -1) {
			fprintf(stderr, "pipe() failed.\n");
			my_exit(1);
		}

		child_pid = fork();
		if (child_pid > 0) {//Parent process
			close(to_child_pipe[0]);
			close(from_child_pipe[1]);

			dup2(newsockfd, 0);
			dup2(newsockfd, 1);
			dup2(newsockfd, 2);

			pthread_mutex_init(&p_lock, NULL);
			pthread_create(&pthread_id, NULL, read_from_child, &from_child_pipe[0]);
			while(1) {
				int i = read(0, buffer, BUFFERSIZE);
				
				if (encrypt == 1) {
					mdecrypt_generic(td, buffer, i);
				}
				int j;
				if (i > 0) {
					if (*buffer == 0x04) {
						my_exit(0);
					} else if(*buffer == 0x03) {
						kill(child_pid, SIGINT);
					} else if (*buffer == 0x0A || *buffer == 0x0D) {
						buffer[0] = '\012';
						j = write(to_child_pipe[1], buffer, 1);
					} else {
						j = write(to_child_pipe[1], buffer, i);
					}
					if (j < 1) {
						my_exit(2);
					}
				} else {
					close(to_child_pipe[0]);
					close(from_child_pipe[0]);
					close(to_child_pipe[1]);
					close(from_child_pipe[1]);
					my_exit(1);
				}
			}
		} else if (child_pid == 0) {//child process
			close(to_child_pipe[1]);
			close(from_child_pipe[0]);
			dup2(to_child_pipe[0], STDIN_FILENO);
			dup2(from_child_pipe[1], STDERR_FILENO);
			dup2(from_child_pipe[1], STDOUT_FILENO);
			close(to_child_pipe[0]);
			close(from_child_pipe[1]);

			char *execvp_argv[2];
			char execvp_filename[] = "/bin/bash";
			execvp_argv[0] = execvp_filename;
			execvp_argv[1] = NULL;
			if (execvp(execvp_filename, execvp_argv) == -1) {
				fprintf(stderr, "execvp() failed.\n");
				my_exit(1);
			}
		} else {//fork failed
			fprintf(stderr, "fork failed.\n");
			my_exit(1);
		}
	}
	return 0;
}
