#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#define MAXSIZE 2048

void sighandler(int signum) {
	//Logs the segmentation fault
	fprintf(stderr, "There was a segmentation fault.");
	exit(3);
}

int main (int argc, char *argv[]) {

	//0 means don't trigger a segmentation fault, 1 means do
	int seg = 0;

	static struct option long_options[] = {
                        {"input", required_argument, 0, 'i'},
                        {"output", required_argument, 0, 'o'},
                        {"segfault", no_argument, 0, 's'},
                        {"catch", no_argument, 0, 'c'},
                        {0, 0, 0, 0}                
	};

	while (1) {
		int option_index = 0;

		int c = getopt_long(argc, argv, "i:o:sc", long_options, &option_index);
		if (c == -1)
			break;
	
		int ifd;
		int ofd;
	
		switch(c) {
		case 'i':
			ifd = open(optarg, O_RDONLY);
			if (ifd >= 0) {
				dup2(ifd,0);
				close(ifd);
			} else {
				//Report the failure
				perror("The following error occured while trying to input: ");	
				exit(1);
			}
			break;

		case 'o':
			ofd = creat(optarg, 0666);
			if (ofd >= 0) {
				dup2(ofd,1);
				close(ofd);
			} else {
				//Report th failure
				perror("The following error occured while trying to output: ");
				exit(2);
			}
			break;

		case 's':
			seg = 1;	
			break;

		case 'c':
			signal(SIGSEGV, sighandler);
			break;
		}
	}


	if (seg == 1) {
		char* temp = NULL;
        	*temp = 1;
        }

	int count;
	char buffer[MAXSIZE] = {};

	while((count = read(0, buffer, MAXSIZE)) > 0) {
		write(1, buffer, count);
	}
	
	exit(0);
}

