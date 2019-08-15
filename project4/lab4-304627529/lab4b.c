#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/aio.h>
#include <time.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>

const int B=4275;
int logfd;
int sockfd;
int portno;
FILE *mLog;

int far = 1;
int stop = 0;
int frequency = 3;

char mOff[15] = "OFF";
char mStop[15] = "STOP";
char mStart[15] = "START";
char mScale[15] = "SCALE";
char mFreq[15] = "FREQ=";

sig_atomic_t volatile run_flag = 1;

void *read_from_server(void *arg) {
        char buffer[257] = {};
	
	while(1) {
		int i = read(sockfd, buffer, 256);
		if (i > 0) {
			buffer[i] = 0;
			if (strcmp(mOff, buffer) == 0) {//OFF
				fprintf(stderr, "OFF\n");
				fprintf(mLog, "OFF\n");
				fflush(mLog);
				exit(1);
			} else if (strcmp(mStop, buffer) == 0) {//STOP
				fprintf(stderr, "STOP\n");
				fprintf(mLog, "STOP\n");
				fflush(mLog);
				stop = 1;
			} else if (strcmp(mStart, buffer) == 0) {//START
				fprintf(stderr, "START\n");
				fprintf(mLog, "START\n");
				fflush(mLog);
				stop = 0;
			} else {
				char temp[6];
				memcpy(temp, &buffer[0], 5);
				temp[5] = '\0';
				if (strcmp(temp, mScale) == 0 && strlen(buffer) == 7) {	
					if (buffer[6] == 'F') {
						fprintf(stderr, "SCALE=F\n");
						fprintf(mLog, "SCALE=F\n"); 
						far = 1;
					} else if (buffer[6] == 'C') {
						fprintf(stderr, "SCALE=C\n");
						fprintf(mLog, "SCALE=C\n");
						far = 0; 
					} else {
						fprintf(stderr, buffer);
						fprintf(stderr, " I\n");
						fprintf(mLog, buffer);
						fprintf(mLog, " I\n");
					}
					fflush(mLog);
				} else if (strcmp(temp, mFreq) == 0 && strlen(buffer) < 10) {
					char temp1[5];
					memcpy(temp1, &buffer[5], strlen(buffer)-5);
					temp1[4] = '\0';
					if (atoi(temp1) > 0 && atoi(temp1) <=3600) {
						frequency = atoi(temp1);
						fprintf(stderr, buffer);
						fprintf(stderr, "\n");
						fprintf(mLog, buffer);
						fprintf(mLog, "\n");
					} else {
						fprintf(stderr, buffer);        
                                                fprintf(stderr, " I\n");          
                                                fprintf(mLog, buffer);       
                                                fprintf(mLog, " I\n");
					}
					fflush(mLog);
				} else {//Invalid command?
					fprintf(stderr, buffer);
					fprintf(stderr, " I\n");
					fprintf(mLog, buffer);
					fprintf(mLog, " I\n");
					fflush(mLog);
				}
			}
		} else {
			perror("ERROR reading from server.\n");
		}
		sleep(frequency);	
	}
}

int main() {
	//Set up server
	struct sockaddr_in serv_addr;
	struct hostent *server;
	portno = 16000;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
	}

	server = gethostbyname("lever.cs.ucla.edu");
	if (server == NULL) {
		fprintf(stderr, "Error, no such host.\n");
		exit(-1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	//Now connect to the server
	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(-1);
	}

	int i;
	char request[23] = {"Port request 304627529"};
	char buffer[257] = {};
	char log_message[256] = {};
	
	i  = write(sockfd, request, strlen(request));
	if (i < 0) {
		perror("write to port 16000 failed.\n");
	}
	int newportno;
	i = read(sockfd, &newportno, sizeof(int));
	if (i < 0) {
		perror("read from port 16000 failed.\n");
	}
	fprintf(stderr, "New port number: %d\n", newportno); 
	close(sockfd);
	portno = newportno;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {            
                perror("ERROR opening socket");  
        }

	serv_addr.sin_port = htons(portno);                                     
                                                                                
        //Now connect to the server                                       
        if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("ERROR connecting");                                     
                exit(-1);                                                       
        } 

	//rest of lab
	pthread_t pthread_id;
	pthread_create(&pthread_id, NULL, read_from_server, &sockfd);	

	uint16_t value;
	mraa_aio_context temp;
	temp = mraa_aio_init(0);

	mLog = fopen("4b_log", "w");
	int x;
	time_t time_1;
	struct tm *time_2;
	char time_str[9];
	while(run_flag) {
		if (stop == 0) {
			time(&time_1);
			time_2 = localtime(&time_1);
			strftime(time_str, 9, "%H:%M:%S", time_2);
			value = mraa_aio_read(temp);
			float R = 1023.0/((float)value)-1.0;
			R = 100000.0*R;
			float temperature;
			if (far == 1) {
				temperature=1.0/(log(R/100000.0)/B+1/298.15)-273.15;
				temperature = temperature * 1.8 + 32;
			} else {
				temperature=1.0/(log(R/100000.0)/B+1/298.15)-273.15;
			}
			if (far == 1) {
				printf("%s %.1fF\n", time_str, temperature);
				fprintf(mLog, "%s %.1fF\n", time_str, temperature);
			} else {
				printf("%s %.1fC\n", time_str, temperature);           
                	        fprintf(mLog, "%s %.1fC\n", time_str, temperature);
			}
			fflush(mLog);
			sprintf(buffer, "304627529 TEMP=%.1f\n", temperature);
			i = write(sockfd, buffer, strlen(buffer));
			if (i < 0) {
				perror("ERROR writing temperature to socket\n");
			}
		}
		sleep(frequency);
	}
	
	mraa_aio_close(temp);

	return 0;
}
