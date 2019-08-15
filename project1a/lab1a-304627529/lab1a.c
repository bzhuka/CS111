#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int shell = 0;
struct termios saved_attributes;
pthread_t pthread_id;
int shell_pid = -1;

pthread_mutex_t p_lock;

char crlf[2] = {0x0D, 0x0A};

void reset_input_mode(void) {
        tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void my_exit(int status) {
        pthread_mutex_lock(&p_lock);
        exit(status);
}

void sighandler(int signum) {
	fprintf(stderr, "Recieved a SIGPIPE from the shell. \n");
	//reset_input_mode();
	my_exit(1);
}

void exit_func() {
	if (shell_pid > 0) {
		reset_input_mode();
	
		int child_status;
		waitpid(shell_pid, &child_status, 0);
	
		if(WIFEXITED(child_status)) {
			fprintf(stderr, "Exit status from normal termination: %d\n", WEXITSTATUS(child_status));
		} else if (WIFSIGNALED(child_status)) 	{
			fprintf(stderr, "Exit status from termination from signal: %d\n", WTERMSIG(child_status));
		}
	}
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

int write_buffer(int file_descriptor, char* buffer) {
	if (*buffer == 0x0D || *buffer == 0x0A) {
		write(file_descriptor, crlf, 2);
	} else if (*buffer == '\004') {
		//close pipe send SIGHUP restore and exit(0)
		if (shell_pid > 0) {
			pthread_cancel(pthread_id);
			close(file_descriptor);
			kill(shell_pid, SIGHUP);
		}
		my_exit(0);
	} else if (*buffer == '\003') {
		kill(shell_pid,SIGINT);
	} else {
		write(file_descriptor, buffer, 1);
	}
	return 0;
}

void *second_thread(void *arg) {
	signal(SIGPIPE, sighandler);
	
	int *pipe = (int*)arg;
	char buffer[1] = {};

	while(1) {
		int i = read(*pipe, buffer, 1);		
		if (i > 0) {
			if (*buffer == '\004') {
				my_exit(1);
			} else {
				write(1, buffer,1);
			}
		} else if (i == 0){
			my_exit(1);
		}
	}
}

int main(int argc, char *argv[]) {
	static struct option long_options[] = {
		{"shell", no_argument, 0, 's'},
		{0, 0, 0, 0}
	};

	while(1) {
		int option_index = 0;

		int c = getopt_long(argc, argv, "s", long_options, &option_index);
		if (c == -1)
			break;

		switch(c) {
		case 's':
			shell = 1;
			break;
		}
	}		

	int to_child_pipe[2];
	int from_child_pipe[2];
	pid_t child_pid = -1;
	char buffer[1] = {};

	set_input_mode();
	atexit(exit_func);

	if (pipe(to_child_pipe) == -1) {
		fprintf(stderr, "pipe() failed.\n");
		my_exit(1);
	}

	if(pipe(from_child_pipe) == -1) {
		fprintf(stderr, "pipe() failed.\n");
		my_exit(1);
	}

	if (shell == 1) {
		child_pid = fork();
		if (child_pid > 0) {//parent process
			shell_pid = child_pid;
			close(to_child_pipe[0]);
			close(from_child_pipe[1]);
				
			pthread_mutex_init(&p_lock, NULL);			
			pthread_create(&pthread_id, NULL, second_thread, &from_child_pipe[0]);
			while(1) {
				int i = read(0, buffer, 1);
				if (i == 1) {
					write_buffer(1, buffer);
					write(to_child_pipe[1], buffer, 1);
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
		} else { //fork failed
			fprintf(stderr, "fork failed.\n");
			my_exit(1);
		}
	}
	while(1) {
		//I don't see how it can ever exceed buffer size
		read (STDIN_FILENO, &buffer, 1);
		write_buffer(1, buffer);
	}
	
	my_exit(0);
}
