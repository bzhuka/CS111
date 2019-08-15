#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/aio.h>
#include <time.h>
#include <math.h>

const int B=4275;

sig_atomic_t volatile run_flag = 1;

int main() {
	uint16_t value;
	mraa_aio_context temp;
	temp = mraa_aio_init(0);

	FILE *mLog;
	mLog = fopen("4a_log", "w");
	int x;
	time_t time_1;
	struct tm *time_2;
	char time_str[9];
	char buffer[512];
	while(run_flag) {
		time(&time_1);
		time_2 = localtime(&time_1);
		strftime(time_str, 9, "%H:%M:%S", time_2);
		value = mraa_aio_read(temp);
		float R = 1023.0/((float)value)-1.0;
		R = 100000.0*R;

		float temperature=1.0/(log(R/100000.0)/B+1/298.15)-273.15;
		printf("%s %.1f\n", time_str, temperature);
		fprintf(mLog, "%s %.1f\n", time_str, temperature);
		fflush(mLog);
		sleep(1);
	}
	
	mraa_aio_close(temp);

	return 0;
}
