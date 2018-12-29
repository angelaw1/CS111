// NAME: Angela Wu
// EMAIL: angelawu.123456789@gmail.com
// ID: 604763501

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/aio.h>
#include <mraa/gpio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BUFFSIZE		256
#define EXIT_FAIL		1
#define EXIT_SUCCESS	0
#define FAHRENHEIT		0
#define CELSIUS			1

struct termios saved_attributes;
// char* buff;
int config_rate = 1;
int temp_unit = FAHRENHEIT; 
int stop_log = 0;
int log_flag = 0;
int log_shutdown  = 0;
int button_pressed = 0;
int out_fd = STDOUT_FILENO;
struct pollfd poll_fd[1];
char print_string[BUFFSIZE];

time_t rawtime;
struct tm *info;

const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k
mraa_aio_context sensor;
mraa_gpio_context button;
sig_atomic_t volatile run_flag = 1;



// prints error message and exits
void syscall_error(void) {
	fprintf(stderr, "Error number: %d\n %s\n", errno, strerror(errno));
	exit(EXIT_FAIL);
}

void usage_error(void) {
	fprintf(stderr, "Correct Usage: ./lab4b [--period=#] [--scale=CF] [--log=filename]\n");
	exit(EXIT_FAIL);
}

void do_when_interrupted() {
	// printf("old time: %02d:%02d:%02d\n", info->tm_hour, info->tm_min, info->tm_sec);
	run_flag = 0;
	log_shutdown = 1;
	stop_log = 0;
	button_pressed = 1;
	/* usleep(config_rate*1000000);
	time(&rawtime);
	info = localtime(&rawtime);
	if (info == NULL) {
		syscall_error();
	}
	printf("new time: %02d:%02d:%02d\n", info->tm_hour, info->tm_min, info->tm_sec); */
}

void gpio_setup(void) {
	sensor = mraa_aio_init(1);
	button = mraa_gpio_init(62);

	sensor = mraa_aio_init(1);
	mraa_gpio_dir(button, MRAA_GPIO_IN);

}

// resets the terminal settings
void reset_input_mode(void) {
	if ((tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes)) == -1) {
		syscall_error();
	}
	printf("reset successful\n");
}

void getLogFile(char* logfile) {
	char temp[BUFFSIZE];
	strcpy(temp, logfile);
	memset(logfile, 0, BUFFSIZE);
	strncpy(logfile, temp + 4, strlen(temp) - 5);
}

int main(int argc, char **argv) {
	// buff = (char*) malloc(BUFFSIZE*sizeof(char));
	char buff[BUFFSIZE];

	struct option longopts[] = {
		{"period", required_argument, NULL, 'p'},
		{"scale", required_argument, NULL, 's'},
		{"log", required_argument, NULL, 'l'},
		{0, 0, 0, 0}
	};

	int opt_ret;
	while((opt_ret = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
		switch(opt_ret) {
			case 'p':
				config_rate = atoi(optarg);
				if (config_rate <= 0) {
					fprintf(stderr, "Error: can't have negative configuration rate");
					exit(EXIT_FAIL);
				}
				break;

			case 's':
				if (strlen(optarg) != 1) 
					usage_error();

				if (optarg[0] == 'F')
					temp_unit = FAHRENHEIT;
				else if (optarg[0] == 'C')
					temp_unit = CELSIUS;
				else
					usage_error();

				break;

			case 'l':
				log_flag = 1;
				if ((out_fd = creat(optarg, S_IRWXU)) == -1) {
					syscall_error();
				}
				break;

			default:
				usage_error();
		}
	}

	gpio_setup();

	mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &do_when_interrupted, NULL);
	uint16_t value;

	int bytes_read = 0;
	int poll_ret;
	poll_fd[0].fd = STDIN_FILENO;
	poll_fd[0].events = POLLIN | POLLERR | POLLHUP;
	char temp_buff[BUFFSIZE];
	memset(temp_buff, 0, BUFFSIZE);
	int temp_buff_i = 0;
	while (run_flag) {
		int timeout = 0;
		poll_ret = poll(poll_fd, 1, timeout);
		if (poll_ret == -1) {
			syscall_error();
		}
		if (poll_fd[0].revents & POLLIN  ||
			poll_fd[0].revents & POLLERR ||
			poll_fd[0].revents & POLLHUP) {

			bytes_read = read(STDIN_FILENO, buff, BUFFSIZE);
			if (bytes_read < 0) {
				syscall_error();
			}

			int i = 0;
			while (i != bytes_read) {
				if (buff[i] == '\n') {
					temp_buff[temp_buff_i] = buff[i];

					if (log_flag) {
						if (write(out_fd, temp_buff, strlen(temp_buff)) == -1)
							syscall_error();
					}

					if (strcmp(temp_buff, "SCALE=F\n") == 0) {
						temp_unit = FAHRENHEIT;
					}
					else if (strcmp(temp_buff, "SCALE=C\n") == 0)  {
						temp_unit = CELSIUS;
					}
					else if (strcmp(temp_buff, "STOP\n") == 0) {
						stop_log = 1;
					}
					else if (strcmp(temp_buff, "START\n") == 0) {
						stop_log = 0;
					}
					else if (strcmp(temp_buff, "OFF\n") == 0) {
						log_shutdown = 1;
						stop_log = 1;
						run_flag = 0;
					}
					else {
						char str[BUFFSIZE];
						memset(str, 0, BUFFSIZE);
						strncpy(str, temp_buff, 7);
						if (strcmp(str, "PERIOD=") == 0) {
							config_rate = atoi(buff + 7);
						}
						memset(str, 0, BUFFSIZE);
						strncpy(str, temp_buff, 4);
						if (strcmp(str, "LOG=") == 0) {
							log_flag = 1;
							getLogFile(temp_buff);
							if ((out_fd = creat(temp_buff, S_IRWXU)) == -1) {
								syscall_error();
							}
						}
					}

					temp_buff_i = 0;
					memset(temp_buff, 0, BUFFSIZE);
				}
				else {
					temp_buff[temp_buff_i] = buff[i];
					temp_buff_i++;
				}
				i++;
			}
		}

		memset(buff, 0, BUFFSIZE);

		time(&rawtime);
		info = localtime(&rawtime);
		if (info == NULL) {
			syscall_error();
		}

		value = mraa_aio_read(sensor);
		float R = 1023.0/value-1.0;
		R = R0*R;

		float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet

		if (temp_unit == FAHRENHEIT)
			temperature = (temperature*(9.0/5.0)) + 32;


		if (!button_pressed && !log_shutdown) {
			sprintf(print_string, "%02d:%02d:%02d %.1f\n", info->tm_hour, info->tm_min, info->tm_sec, temperature);
			if (!stop_log) {
				if (log_flag) {
					if (write(out_fd, print_string, strlen(print_string)) == -1) {
						syscall_error();
					}
				}
				else {
					printf("%s", print_string);
				}			
			}
			usleep(config_rate*1000000);

		}
	}

	if (button_pressed) {
		// usleep(config_rate*1000000);
		time(&rawtime);
		info = localtime(&rawtime);
		if (info == NULL) {
			syscall_error();
		}
	}
	sprintf(print_string, "%02d:%02d:%02d SHUTDOWN\n", info->tm_hour, info->tm_min, info->tm_sec);
	if (log_flag) {
		if (write(out_fd, print_string, strlen(print_string)) == -1) {
			syscall_error();
		}
	}
	else {
		printf("%s", print_string);
	}	

	mraa_aio_close(sensor);
	mraa_gpio_close(button);
	close(out_fd);

}
