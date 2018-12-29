// NAME: Angela Wu
// EMAIL: angelawu.123456789@gmail.com
// ID: 604763501

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>		// contains constants and structures needed for internet domain addresses
#include <arpa/inet.h>
#include <assert.h>
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define BUFFSIZE		256
#define COMPRESSSIZE 	4096
#define EXIT_FAIL		1
#define EXIT_SUCCESS	0

struct termios saved_attributes;
unsigned char in[BUFFSIZE];		// holds data read from the socket
unsigned char out[BUFFSIZE];		// holds data that will be sent through the socket
unsigned char compression_buff[COMPRESSSIZE];
int socket_fd;
struct pollfd poll_fd[2];
int log_fd;
char *logfile;

int port_specified = 0;		// set when '--port' option is used
int compress_flag = 0;		// set when '--compress' options used
int log_flag = 0;			// set when '--log' option is used

// prints error message and exits
void print_error_exit(void) {
	fprintf(stderr, "Error number: %d %s\n", errno, strerror(errno));
	exit(EXIT_FAIL);
}

// resets the terminal settings
void reset_input_mode(void) {
	if ((tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes)) == -1) {
		print_error_exit();
	}
}

// changes terminal settings
void set_input_mode(void) {
	
	struct termios tattr;

	/* if(!isatty(STDIN_FILENO)) {
		fprintf(stderr, "Not a terminal");
		exit(EXIT_FAIL);
	} */

	if (tcgetattr(STDIN_FILENO, &saved_attributes) == -1) {
		print_error_exit();
	}

	if (tcgetattr(STDIN_FILENO, &tattr) == -1) {
		print_error_exit();
	}

	if (atexit(reset_input_mode) != 0) {
		print_error_exit();
	}

	tattr.c_iflag = ISTRIP;
	tattr.c_oflag = 0;
	tattr.c_lflag = 0;

	if ((tcsetattr(STDIN_FILENO, TCSANOW, &tattr)) == -1) {
		print_error_exit();
	}
}

void read_write(void) {
	int bytes_read_keyboard;
	int bytes_read_socket;
	int poll_ret;

	poll_fd[0].fd = STDIN_FILENO;		// KEYBOARD
	poll_fd[0].events = POLLIN | POLLERR | POLLHUP;
	poll_fd[1].fd = socket_fd;			// SHELL
	poll_fd[1].events = POLLIN | POLLERR | POLLHUP;
	
	int keyboard_pflag = 0;		// gets set when there is input from keyboard
	int socket_pflag = 0;		// gets set when there is input from shell

	while (1) {

		bzero(in, BUFFSIZE);
		bzero(out, BUFFSIZE);
		bzero(compression_buff, COMPRESSSIZE);

		int timeout = 0;
		poll_ret = poll(poll_fd, 2, timeout);

		if (poll_ret == -1) {
			print_error_exit();
		}

		if (poll_ret > 0) {

			if (poll_fd[0].revents && POLLIN  /*||
				poll_fd[0].revents & POLLERR ||
				poll_fd[0].revents & POLLHUP*/) {
				keyboard_pflag = 1;
			}
			if (poll_fd[1].revents && POLLIN  /*||
				poll_fd[1].revents & POLLERR ||
				poll_fd[1].revents & POLLHUP*/) {
				socket_pflag = 1;

			}

		}

		// if there is input from the keyboard
		if (keyboard_pflag == 1) {
			bytes_read_keyboard = read(STDIN_FILENO, out, BUFFSIZE);

			if (bytes_read_keyboard < 0) {
				print_error_exit();
			}

			// DEFLATE: received uncompressed file, out -> compression file
			if (compress_flag) {
				int i = 0;
				while(i != bytes_read_keyboard) {
					if (out[i] == '\r' || out[i] == '\n') {
						if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
							print_error_exit();
						}
					}
					else if (write(STDOUT_FILENO, out + i, 1) == -1) {
						print_error_exit();
					}
					i++;
				}

				z_stream to_socket;

				to_socket.zalloc = Z_NULL;
				to_socket.zfree = Z_NULL;
				to_socket.opaque = Z_NULL;

				deflateInit(&to_socket, Z_DEFAULT_COMPRESSION);

				to_socket.avail_in = bytes_read_keyboard;
				to_socket.next_in = out;
				to_socket.avail_out = COMPRESSSIZE;
				to_socket.next_out = compression_buff;

				do {
					deflate(&to_socket, Z_SYNC_FLUSH);
				} while (to_socket.avail_in > 0);

				write(socket_fd, compression_buff, COMPRESSSIZE - to_socket.avail_out);

				bytes_read_keyboard = COMPRESSSIZE - to_socket.avail_out;

				deflateEnd(&to_socket);					
			
			}

			else {
				// process each character
				int i = 0;
				while (i != bytes_read_keyboard) {

					if (out[i] == '\r' || out[i] == '\n') {
						if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
							print_error_exit();
						}
					}
					if (write(socket_fd, out + i, 1) == -1 || write(STDOUT_FILENO, out + i, 1) == -1) {
						print_error_exit();
					}
					i++;
				}
			}

			if (log_flag) {
				char temp[BUFFSIZE + 20];
				bzero(temp, sizeof(temp));
				if (!compress_flag && sprintf(temp, "SENT %d bytes: %s\n", bytes_read_keyboard, out) < 0) {
					print_error_exit();
				}
				else if (compress_flag && sprintf(temp, "SENT %d bytes: %s\n", bytes_read_keyboard, compression_buff) < 0) {
					print_error_exit();
				}
				int i;
				for (i = 0; temp[i] != '\0'; i++) {
					if (write(log_fd, temp + i, 1) == -1) {
						print_error_exit();
					}					
				}

			}
		}

		// if there is input from the socket
		if (socket_pflag == 1) {
			if (compress_flag) {
				bytes_read_socket = read(socket_fd, compression_buff, COMPRESSSIZE);
			}
			else {
				bytes_read_socket = read(socket_fd, in, BUFFSIZE);
			}

			if (bytes_read_socket < 0) {
				print_error_exit();
			}

			// received EOF from socket
			if (bytes_read_socket == 0) {
				exit(EXIT_SUCCESS);
			}
				
			if (log_flag) {
				char temp[BUFFSIZE + 20];
				bzero(temp, sizeof(temp));
				if (!compress_flag && sprintf(temp, "RECEIVED %d bytes: %s\n", bytes_read_socket, in) < 0) {
					print_error_exit();
				}
				else if (compress_flag && sprintf(temp, "RECEIVED %d bytes: %s\n", bytes_read_socket, compression_buff) < 0) {
					print_error_exit();
				}
				int i;
				for (i = 0; temp[i] != '\0'; i++) {
					if (write(log_fd, temp + i, 1) == -1) {
						print_error_exit();
					}
				}
			}

			// INFLATE: compressed file, compression_buff -> in
			if (compress_flag) {
				z_stream from_socket;

				from_socket.zalloc = Z_NULL;
				from_socket.zfree = Z_NULL;
				from_socket.opaque = Z_NULL;

				inflateInit(&from_socket);

				from_socket.avail_in = bytes_read_socket;
				from_socket.next_in = compression_buff;
				from_socket.avail_out = BUFFSIZE;
				from_socket.next_out = in;

				do {
					inflate(&from_socket, Z_SYNC_FLUSH);
				} while (from_socket.avail_in > 0);

				bytes_read_socket = BUFFSIZE - from_socket.avail_out;
				inflateEnd(&from_socket);
			}

			// if (!log_flag) {
				// process each character
				int i = 0;
				while (i != bytes_read_socket) {
					if (in[i] == '\n' || in[i] == '\r') {
						if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
							print_error_exit();
						}
					}
					else if (write(STDOUT_FILENO, in + i, 1) == -1) {
						print_error_exit();
					}
					i++;
				}
			// }
		}

		keyboard_pflag = 0;
		socket_pflag = 0;
	}
}

int main(int argc, char **argv) {

	int port_num;
	struct sockaddr_in server_address;

	// checks for correct number of arguments
	if (argc < 2) {
		fprintf(stderr, "Correct Usage: %s --port=port# [--log=filename] [--compress]\n", argv[0]);
		exit(EXIT_FAIL);
	}

	struct option longopts[] = {
		{"port", required_argument, NULL, 'p'},
		{"log", required_argument, NULL, 'l'},
		{"compress", no_argument, NULL, 'c'},
		{0, 0, 0, 0}
	};

	int opt_ret;
	while((opt_ret = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
		switch(opt_ret) {

			case 'p':
				port_num = atoi(optarg);
				port_specified = 1;
				break;

			case 'l':
				log_flag = 1;
				logfile = optarg;
				break;

			case 'c':
				compress_flag = 1;
				break;

			default:
				fprintf(stderr, "Correct Usage: lab1b --port=port# [--log=filename] [--compress]\n");
				exit(EXIT_FAIL);
				break;

		}
	}

	// checks if port number was given
	if (!port_specified) {
		fprintf(stderr, "Correct Usage: lab1b --port=port# [--log=filename] [--compress]\n");
		exit(EXIT_FAIL);
	}

	if (log_flag == 1) {
		if ((log_fd = creat(logfile, S_IRWXU)) < 0) {
			print_error_exit();
		}
	}

	set_input_mode();

	// create a socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		print_error_exit();
	}

	// set up parameters for the connect syscall
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port_num);
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
		print_error_exit();
	}

	read_write();

	exit(EXIT_SUCCESS);
}