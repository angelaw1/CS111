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
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>		// contains constants and structures needed for internet domain addresses
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netdb.h>

#define BUFFSIZE			256
#define EXIT_SUCCESS		0
#define EXIT_PARAMETERS		1
#define EXIT_RUN_TIME_FAIL	2
#define FAHRENHEIT			0
#define CELSIUS				1

int config_rate = 1;
int temp_unit = FAHRENHEIT;
int log_flag = 0;
int id_flag = 0;
int host_flag = 0;
int port_flag = 0;

int out_fd = STDOUT_FILENO;
int id_num = 604763501;
int in_fd;
int port;
int socket_fd;
struct sockaddr_in client;
char *hostname;

const SSL_METHOD *method;
SSL_CTX *ctx;
SSL *ssl;

int stop_log = 0;
int log_shutdown  = 0;

struct pollfd poll_fd[1];
char print_string[BUFFSIZE];

time_t rawtime;
struct tm *info;

const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k
mraa_aio_context sensor;
sig_atomic_t volatile run_flag = 1;



// prints error message and exits
void syscall_error(void) {
	fprintf(stderr, "Error number: %d\n %s\n", errno, strerror(errno));
	exit(EXIT_RUN_TIME_FAIL);
}

void usage_error(void) {
	fprintf(stderr, "Correct Usage: ./lab4b --id=9-digit-number --host=name or address --log=filename port_number [--period=#] [--scale=CF]\n");
	exit(EXIT_PARAMETERS);
}

void socket_setup(void) {
	// create a socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		syscall_error();
	}

	// set up parameters for the connect syscall
	client.sin_family = AF_INET;
	client.sin_port = htons(port);
	struct hostent *server = gethostbyname(hostname);
 	if(server == NULL){
    	fprintf(stderr, "no such host\n");
    	exit(1);
	}

	memcpy( (char *) &client.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	if (connect(socket_fd, (struct sockaddr *) &client, sizeof(client)) == -1) {
		syscall_error();
	}
}

void ssl_setup(void) {
	OpenSSL_add_all_algorithms();
	if (SSL_library_init() < 0) {
		fprintf(stderr, "Could not initialize the OpenSSL library\n");
		exit(EXIT_RUN_TIME_FAIL);
	}
	method = SSLv23_client_method();
	if ((ctx = SSL_CTX_new(method)) == NULL) {
		fprintf(stderr, "Uable to create a new SSL context structure\n");
		exit(EXIT_RUN_TIME_FAIL);
	}
	ssl = SSL_new(ctx);
	if (SSL_set_fd(ssl, socket_fd) == 0) {
		fprintf(stderr, "Unable to attach the SSL session to the socket descriptor\n");
		exit(EXIT_RUN_TIME_FAIL);
	}
	if (SSL_connect(ssl) != 1) {
		fprintf(stderr, "Error: Could not build a SSL session\n");
		exit(EXIT_RUN_TIME_FAIL);
	}
}

void gpio_read_write(void) {
	uint16_t value;

	char buff[BUFFSIZE];								// buffer to read in commands
	int bytes_read = 0;
	char temp_buff[BUFFSIZE];							// stores strings before \n
	int temp_buff_i = 0;
	memset(temp_buff, 0, BUFFSIZE);

	int poll_ret;
	poll_fd[0].fd = socket_fd;
	poll_fd[0].events = POLLIN | POLLERR | POLLHUP;

	while (run_flag) {
		memset(buff, 0, BUFFSIZE);

		// get temperature
		int timeout = 0;
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

		// writes temperature to log and socket
		if (!log_shutdown) {
			sprintf(print_string, "%02d:%02d:%02d %.1f\n", info->tm_hour, info->tm_min, info->tm_sec, temperature);
			if (!stop_log) {
				if (write(out_fd, print_string, strlen(print_string)) == -1) {
					syscall_error();
				}
				if (SSL_write(ssl, print_string, strlen(print_string)) == 0) {
					fprintf(stderr, "Failed to write to SSL connection\n");
					exit(EXIT_RUN_TIME_FAIL);
				}
			}
			usleep(config_rate*1000000);

		}

		poll_ret = poll(poll_fd, 1, timeout);
		if (poll_ret == -1) {
			syscall_error();
		}
		if (poll_fd[0].revents & POLLIN  ||
			poll_fd[0].revents & POLLERR ||
			poll_fd[0].revents & POLLHUP) {

			bytes_read = SSL_read(ssl, buff, BUFFSIZE);
			if (bytes_read == 0) {
				fprintf(stderr, "Failed to read from SSL connection\n");
				exit(EXIT_RUN_TIME_FAIL);
			}

			int i = 0;
			while (i != bytes_read) {
				if (buff[i] == '\n') {
					temp_buff[temp_buff_i] = buff[i];

					if (write(out_fd, temp_buff, strlen(temp_buff)) == -1)
						syscall_error();

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
						if (strcmp(str, "LOG ") == 0) {
							// do nothing, automatically logged
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
	}

	sprintf(print_string, "%02d:%02d:%02d SHUTDOWN\n", info->tm_hour, info->tm_min, info->tm_sec);
	if (write(out_fd, print_string, strlen(print_string)) == -1) {
		syscall_error();
	}
	if (SSL_write(ssl, print_string, strlen(print_string)) == 0) {
		fprintf(stderr, "Failed to write to SSL connection\n");
		exit(EXIT_RUN_TIME_FAIL);
	}
}

int main(int argc, char **argv) {

	if (argc < 5) {
		usage_error();
	}

	struct option longopts[] = {
		{"period", required_argument, NULL, 'p'},
		{"scale", required_argument, NULL, 's'},
		{"log", required_argument, NULL, 'l'},
		{"id", required_argument, NULL, 'i'},
		{"host", required_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};

	int opt_ret;
	while((opt_ret = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
		switch(opt_ret) {
			case 'p':
				config_rate = atoi(optarg);
				if (config_rate <= 0) {
					fprintf(stderr, "Error: can't have negative configuration rate");
					exit(EXIT_PARAMETERS);
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

			case 'i':
				if (strlen(optarg) != 9) {
					fprintf(stderr, "Error: id number must be 9 digits\n");
					exit(EXIT_PARAMETERS);
				}
				id_num = atoi(optarg);
				id_flag = 1;
				break;

			case 'h':
				hostname = optarg;
				host_flag = 1;
				break;

			default:
				usage_error();
		}
	}

	sensor = mraa_aio_init(1);
	if (sensor == NULL) {
		fprintf(stderr, "Error: Failed to initialize sensor\n");
		exit(EXIT_RUN_TIME_FAIL);
	}

	// find non-option argument (port number)
	port = atoi(argv[optind]);
	if (argv[optind] == NULL) {
		usage_error();
	}
	else {
		port_flag = 1;
	}

	if (!log_flag || !id_flag || !host_flag || !port_flag) {
		usage_error();
	}

	socket_setup();
	ssl_setup();
	char id_string[BUFFSIZE];
	sprintf(id_string, "ID=%d\n", id_num);
	if (SSL_write(ssl, id_string, strlen(id_string)) == 0) {
		fprintf(stderr, "Failed to write to SSL connection\n");
		exit(EXIT_RUN_TIME_FAIL);
	}
	if (write(out_fd, id_string, strlen(id_string)) == -1) {
		syscall_error();
	}

	gpio_read_write();

	SSL_free(ssl);
	SSL_CTX_free(ctx);
	mraa_aio_close(sensor);
	close(out_fd);
	close(socket_fd);
	exit(EXIT_SUCCESS);
}