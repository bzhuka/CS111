#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

#include <unistd.h>
#include <getopt.h>
#include <termios.h>
#include <pthread.h>
#include <fcntl.h>
#include <mcrypt.h>

#define PORTNO 121345
#define LOGSIZE 256
#define BUFFERSIZESERVER 256
#define BUFFERSIZECLIENT 1

MCRYPT td;
int logfd = -1;
int encrypt = 0;
struct termios saved_attributes;
pthread_mutex_t p_lock;

char crlf[2] = {0x0D, 0x0A};

void reset_input_mode(void) {
	tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void my_exit(int status) {
	pthread_mutex_lock(&p_lock);
	exit(status);
}

void set_input_mode(void) {
	struct termios tattr;

	if (!isatty (STDIN_FILENO)) {
		fprintf(stderr, "Not a terminal.\n");
		my_exit (EXIT_FAILURE);
	}

	tcgetattr (STDIN_FILENO, &saved_attributes);
	atexit (reset_input_mode);

	tcgetattr (STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON|ECHO);
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

void *read_from_server(void *arg) {
	int *sockfd = (int*)arg;
	char buffer[BUFFERSIZESERVER + 1] = {};
	char log[LOGSIZE] = {};

	while(1) {
		int i = read(*sockfd, buffer, BUFFERSIZESERVER);
		if (i > 0) {
			buffer[i] = 0;
			if(logfd >= 0) {
				int size = sprintf(log, "RECIEVED %d BYTES: %s\n", i, buffer);
				write(logfd, log, size);
			}

			if (encrypt == 1) {
				mdecrypt_generic(td, buffer, i);
			}			

			int j = write(1, buffer, i);
			if (j <= 0) {
				my_exit(1);
			}
		} else {
			my_exit(1);
		}
	}
}

int main(int argc, char *argv[]) {
	int sockfd, portno;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	portno = PORTNO;

	static struct option long_options[] = {
		{"port", required_argument, 0, 'p'},
		{"encrypt", no_argument, 0, 'e'},
		{"log", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};

	while(1) {
		int option_index = 0;

		int c = getopt_long(argc, argv, "p:el:", long_options, &option_index);
		if (c == -1)
			break;

		switch(c) {
			case 'p':
				portno = atoi(optarg);
				break;
			case 'l':
				logfd = creat(optarg, 0666);
				break;
			case 'e':
				encrypt = 1;
				break;
		}
	}

	set_input_mode();

	/* Create a socket point */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	server = gethostbyname("localhost");

	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(1);
	}

	if(encrypt == 1) {
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
		 *  * consider using /dev/random or /dev/urandom.
		 *   */
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

	pthread_t pthread_id;
	pthread_mutex_init(&p_lock, NULL);
	pthread_create(&pthread_id, NULL, read_from_server, &sockfd);

	char buffer[BUFFERSIZESERVER + 1] = {};
	char log[LOGSIZE] = {};

	while(1) {
		int i = read(0, buffer, BUFFERSIZESERVER);
		buffer[i] = 0;
		if( i > 0) {
			if (*buffer == '\004') {
				my_exit(0);
			} else if (*buffer == 0x0D || *buffer == 0x0A) {
				buffer[0] = 0x0A;
				buffer[1] = 0;
				write(1, crlf, 2);
			} else {
				write(1, buffer, strlen(buffer));
			}
		}

		if(encrypt == 1) {
			mcrypt_generic(td, buffer, i);//strlen(buffer));
		}

		int j = write(sockfd, buffer, strlen(buffer));
		if(j > 0) {
			if(logfd > 0) {
				int size = sprintf(log, "SENT %d BYTES: %s\n", j, buffer);
				write(logfd, log, size);
			}
		}
	}	

	return 0;
}
