// NAME: Angela Wu
// EMAIL: angelawu.123456789@gmail.com
// ID: 604763501

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
#include <netinet/in.h>		// contains constants and structures needed for itnernet domain addresses
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

unsigned char in[BUFFSIZE];							// holds data read from the socket
unsigned char out[BUFFSIZE];						// holds data that will be sent through the socket
unsigned char compression_buff[COMPRESSSIZE];
int pipe1[2];
int pipe2[2];
pid_t childPID;
struct pollfd poll_fd[2];
int socket_fd;
int new_socket;
int compress_flag = 0;								// set when '--compress' options used

void exit_read_from_shell(void) {
	if (close(pipe2[0]) == -1) {
		fprintf(stderr, "Error number: %d\n %s\n", errno, strerror(errno));
		exit(EXIT_FAIL);
	}
}

// prints error message and exits
void print_error_exit(void) {
	fprintf(stderr, "Error number: %d\n %s\n", errno, strerror(errno));
	exit(EXIT_FAIL);
}

// waits for child process and exits
void wait_exit(void) {
	int wstatus;										// waitpid status
	if (waitpid(childPID, &wstatus, 0) == -1) {
		print_error_exit();
	}
	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", 0x007f&wstatus, (0xff00&wstatus) >> 8);
	if (close(new_socket) == -1) {
		print_error_exit();
	}
	exit(EXIT_SUCCESS);
}

// signal handler for SIGPIPE
void handler(int sig) {
	if (sig == SIGPIPE) {
		exit_read_from_shell();
	    int wstatus;										// waitpid status
		if (waitpid(childPID, &wstatus, 0) == -1) {
			print_error_exit();
		}
		fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", 0x007f&wstatus, (0xff00&wstatus) >> 8);
		exit(EXIT_SUCCESS);
  	}
}

// execute shell
void exec_shell(void) {
	char * args[1];
	args[0] = NULL;
	if (execvp("/bin/bash", args) == -1) {
		fprintf(stderr, "Error executing shell");
		exit(EXIT_FAIL);
	}

}

void read_write(void) {
	int bytes_read_shell;
	int bytes_read_socket;
	int poll_ret;
	int closed_pipe = 0;

	poll_fd[0].fd = pipe2[0];							// SHELL
	poll_fd[0].events = POLLIN | POLLERR | POLLHUP;
	poll_fd[1].fd = new_socket;							// SOCKET
	poll_fd[1].events = POLLIN | POLLERR | POLLHUP;
	
	int shell_pflag = 0;		// gets set when there is input from shell
	int socket_pflag = 0;		// gets set when there is input from socket
	// int shell_err_hup = 0;
	// int socket_err_hup = 0;

	while(1) {

		int timeout = 0;
		poll_ret = poll(poll_fd, 2, timeout);

		if (poll_ret == -1) {
			print_error_exit();
		}

		if (poll_ret > 0) {

			if ((poll_fd[0].revents && POLLIN)  ||
				(poll_fd[0].revents && POLLERR) ||
				(poll_fd[0].revents && POLLHUP)) {
				shell_pflag = 1;
			}
			if ((poll_fd[1].revents && POLLIN) ||
				(poll_fd[1].revents && POLLERR) ||
				(poll_fd[1].revents && POLLHUP)) {
				socket_pflag = 1;
			}
		}

		// if there is input from the shell
		if (shell_pflag == 1) {

			bzero(in, BUFFSIZE);
			bzero(out, BUFFSIZE);
			bzero(compression_buff, COMPRESSSIZE);		

			bytes_read_shell = read(poll_fd[0].fd, out, BUFFSIZE);

			if (bytes_read_shell < 0) {
				exit_read_from_shell();
				exit(EXIT_FAIL);
			}

			// received EOF
			if (bytes_read_shell == 0) {
				exit_read_from_shell();

				int wstatus;										// waitpid status
				if (waitpid(childPID, &wstatus, 0) == -1) {
					print_error_exit();
				}
				fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", 0x007f&wstatus, (0xff00&wstatus) >> 8);
				if (close(new_socket) == -1) {
					print_error_exit();
				}
				exit(EXIT_SUCCESS);
			}

			// DEFLATE: received uncompressed file, out -> compression file
			if (compress_flag) {

				// prints to the screen so I can check if the server is geting 
				// correct output from the shell
				int i = 0;
				while (i != bytes_read_shell) {
					if (write(STDOUT_FILENO, out + i, 1) == -1) {
						print_error_exit();
					}
					i++;
				}

				z_stream to_socket;

				to_socket.zalloc = Z_NULL;
				to_socket.zfree = Z_NULL;
				to_socket.opaque = Z_NULL;

				deflateInit(&to_socket, Z_DEFAULT_COMPRESSION);

				to_socket.avail_in = bytes_read_shell;
				to_socket.next_in = out;
				to_socket.avail_out = COMPRESSSIZE;
				to_socket.next_out = compression_buff;

				do {
					deflate(&to_socket, Z_SYNC_FLUSH);
				} while (to_socket.avail_in > 0);

				write(new_socket, compression_buff, COMPRESSSIZE - to_socket.avail_out);

				deflateEnd(&to_socket);

			}	

			else {
				// process each character
				int i = 0;
				while (i != bytes_read_shell) {
					if (write(new_socket, out + i, 1) == -1 || write(STDOUT_FILENO, out + i, 1) == -1) {
						print_error_exit();
					}
					i++;
				}
			}
		}

		// if there is input from the socket
		if (socket_pflag == 1) {

			bzero(in, BUFFSIZE);
			bzero(out, BUFFSIZE);
			bzero(compression_buff, COMPRESSSIZE);

			if (compress_flag) {
				bytes_read_socket = read(poll_fd[1].fd, compression_buff, COMPRESSSIZE);
			}
			else {
				bytes_read_socket = read(poll_fd[1].fd, in, BUFFSIZE);
			}

			if (bytes_read_socket == -1) {
				print_error_exit();
			} 

			// received EOF or read error
			else if (bytes_read_socket == 0) {
				if (closed_pipe == 0) {
					if (close(pipe1[1]) == -1) {
						print_error_exit();
					}
				}
				closed_pipe = 1;
			}

			// INFLATE: received compressed string, compression_buff -> in
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

			// process each character
			int i = 0;
			while (i != bytes_read_socket) {

				if (in[i] == 0x03) {
					if (kill(childPID, SIGINT) == -1) {
						print_error_exit();
					}
				}
				else if (in[i] == 0x04) {
					if (closed_pipe == 0) {
						if (close(pipe1[1]) == -1) {
							print_error_exit();
						}
					}
					closed_pipe = 1;
				}
				else if (in[i] == '\n' || in[i] == '\r') {
					if ((!closed_pipe && write(pipe1[1], "\n", 1) == -1) || write(STDOUT_FILENO, "\n", 1) == -1) {
						print_error_exit();
					}
				}
				else if ((!closed_pipe && write(pipe1[1], in + i, 1) == -1) || write(STDOUT_FILENO, in + i, 1) == -1) {
					print_error_exit();
				}

				i++;
			}
		}

		shell_pflag = 0;
		// shell_err_hup = 0;
		socket_pflag = 0;
		// socket_err_hup = 0;

	}
}

int main(int argc, char **argv) {

	int port_specified = 0;		// set when '--port' option is used
	int port_num;
	struct sockaddr_in server_address;

	struct option longopts[] = {
		{"port", required_argument, NULL, 'p'},
		{"compress", no_argument, NULL, 'c'},
		{0, 0, 0, 0}
	};

	// checks for correct number of arguments
	if (argc < 2) {
		fprintf(stderr, "Correct Usage: %s --port=port# [--log=filename] [--compress]\n", argv[0]);
		exit(EXIT_FAIL);
	}

	int opt_ret;
	while((opt_ret = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
		switch(opt_ret) {
			case 'p':
				port_specified = 1;
				port_num = atoi(optarg);
				break;
			case 'c':
				compress_flag = 1;
				break;
			default:
				fprintf(stderr, "Correct Usage: lab1b --port=port#\n");
				exit(EXIT_FAIL);
				break;
		}
	}

	// checks if port number was given
	if (!port_specified) {
		fprintf(stderr, "Correct Usage: lab1b --port=port# [--log=filename] [--compress]\n");
		exit(EXIT_FAIL);
	}

	// create a socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		print_error_exit();
	}

	// set up parameters for the bind syscall
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port_num);
	server_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(socket_fd, (struct sockaddr *) &server_address,
			 sizeof(server_address)) < 0) {
		print_error_exit();
	}
	if (listen(socket_fd, 1) == -1) {
		print_error_exit();
	}
	if ((new_socket = accept(socket_fd, (struct sockaddr *) NULL, NULL)) == -1) {
		print_error_exit();
	}

	signal(SIGPIPE, handler);

	if (pipe(pipe2) == -1) {
		print_error_exit();
	}
	if (pipe(pipe1) == -1) {
		print_error_exit();
	}

	childPID = fork();
	if (childPID == -1) {
		print_error_exit();
	}

	// CHILD PROCESS
	if (childPID == 0) {
		if (dup2(pipe1[0], STDIN_FILENO) == -1) {
			print_error_exit();
		}
		if (close(pipe1[1]) == -1) {
			print_error_exit();
		}
		if (dup2(pipe2[1], STDOUT_FILENO) == -1) {
			print_error_exit();
		}
		if (dup2(pipe2[1], STDERR_FILENO) == -1) {
			print_error_exit();
		}
		if (close(pipe2[0]) == -1) {
			print_error_exit();
		}
		exec_shell();
	}

	// PARENT PROCESS
	else {
		if (close(pipe1[0]) == -1) {
			print_error_exit();
		}
		if (close(pipe2[1]) == -1) {
			print_error_exit();
		}
		read_write();
	}

	exit_read_from_shell();
	exit(EXIT_SUCCESS);
}