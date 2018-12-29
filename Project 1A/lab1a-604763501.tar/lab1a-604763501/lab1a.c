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

#define BUFFSIZE		256
#define EXIT_FAIL		1
#define EXIT_SUCCESS	0

struct termios saved_attributes;
char* buff;
int pipe1[2];
int pipe2[2];
int shell_flag = 0;
pid_t childPID;

struct pollfd poll_fd[2];

// signal handler for SIGPIPE
void handler(int sig) {
	if (sig == SIGPIPE) {
    	exit(EXIT_SUCCESS);
  	}
}

// prints error message and exits
void print_error_exit(void) {
	fprintf(stderr, "Error number: %d\n %s\n", errno, strerror(errno));
	exit(EXIT_FAIL);
}

// resets the terminal settings
void reset_input_mode(void) {
	if ((tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes)) == -1) {
		print_error_exit();
	}
	printf("reset successful\n");
}

// changes terminal settings
void set_input_mode(void) {
	
	struct termios tattr;

	if(!isatty(STDIN_FILENO)) {
		fprintf(stderr, "Not a terminal");
		exit(EXIT_FAIL);
	}

	if (tcgetattr(STDIN_FILENO, &saved_attributes) == -1) {
		print_error_exit();
	}

	if (tcgetattr(STDIN_FILENO, &tattr) == -1) {
		print_error_exit();
	}

	atexit(reset_input_mode);

	tattr.c_iflag = ISTRIP;
	tattr.c_oflag = 0;
	tattr.c_lflag = 0;

	if ((tcsetattr(STDIN_FILENO, TCSANOW, &tattr)) == -1) {
		printf("fail\n");
		print_error_exit();
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

// read and write function for when '--shell' is not specified
void read_write(void) {
	int bytes_read;
	while (1) {

		bytes_read = read(STDIN_FILENO, buff, BUFFSIZE);
		if (bytes_read < 0) {
			print_error_exit();
		}

		// process each character
		int i = 0;
		while (i != bytes_read) {

			// ctrl-D
			if (buff[i] == 0x04) {
				exit(0);
			}

			// ctrl-C
			else if (buff[i] == '\r' || buff[i] == '\n') {
				if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
					print_error_exit();
				}
			}

			else if (write(STDOUT_FILENO, buff + i, 1) == -1) {
				print_error_exit();
			}

			i += sizeof(char);
		}
	}
}

// read and write function for when '--shell' is specified
void read_write_shell(void) {
	int bytes_read_keyboard;
	int bytes_read_shell;
	int poll_ret;

	poll_fd[0].fd = STDIN_FILENO;		// KEYBOARD
	poll_fd[0].events = POLLIN | POLLERR | POLLHUP;
	poll_fd[1].fd = pipe2[0];			// SHELL
	poll_fd[1].events = POLLIN | POLLERR | POLLHUP;
	
	int keyboard_pflag = 0;		// gets set when there is input from keyboard
	int shell_pflag = 0;		// gets set when there is input from shell

	while (1) {
		int timeout = 0;
		poll_ret = poll(poll_fd, 2, timeout);

		if (poll_ret == -1) {
			print_error_exit();
		}

		if (poll_ret > 0) {

			if (poll_fd[0].revents & POLLIN  ||
				poll_fd[0].revents & POLLERR ||
				poll_fd[0].revents & POLLHUP) {
				keyboard_pflag = 1;
			}
			if (poll_fd[1].revents & POLLIN  ||
				poll_fd[1].revents & POLLERR ||
				poll_fd[1].revents & POLLHUP) {
				shell_pflag = 1;

			}

		}

		// if there is input from the keyboard
		if (keyboard_pflag == 1) {
			bytes_read_keyboard = read(STDIN_FILENO, buff, BUFFSIZE);
			if (bytes_read_keyboard < 0) {
				print_error_exit();
			}

			// process each character
			int i = 0;
			while (i != bytes_read_keyboard) {

				// ctrl-D
				if (buff[i] == 0x04) {
					if (close(pipe1[1]) == -1) {
						print_error_exit();
					}
				}

				// ctrl-C
				else if (buff[i] == 0x03) {
					if (kill(childPID, SIGINT) == -1) {
						print_error_exit();
					}
				}

				// received <cr> or <lf> from keyboard
				else if (buff[i] == '\r' || buff[i] == '\n') {
					if (write(pipe1[1], "\n", 1) == -1 || write(STDOUT_FILENO, "\r\n", 2) == -1) {
						print_error_exit();
					}
				}

				// all other characters
				else if (write(pipe1[1], buff + i, 1) == -1 || write(STDOUT_FILENO, buff + i, 1) == -1) {
					print_error_exit();
				}
			
				i++;

			}
		}

		// if there is input from the shell
		if (shell_pflag == 1) {
			bytes_read_shell = read(pipe2[0], buff, BUFFSIZE);
			if (bytes_read_shell < 0) {
				print_error_exit();
			}

			// received EOF
			else if (bytes_read_shell == 0) {
				int wstatus;
				if (waitpid(childPID, &wstatus, 0) == -1) {
					print_error_exit();
				}
				fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", 0x007f & wstatus, (0xff00 & wstatus) >> 8);
				exit(EXIT_SUCCESS);
			}

			// process each character
			int i = 0;
			while (i != bytes_read_shell) {
				if (buff[i] == '\n') {
					if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
						print_error_exit();
					}
				}

				else if (write(STDOUT_FILENO, buff + i, 1) == -1) {
					print_error_exit();
				}

				i++;
			}
		}

		keyboard_pflag = 0;
		shell_pflag = 0;

	}
}

int main(int argc, char **argv) {
	buff = (char*) malloc(BUFFSIZE*sizeof(char));
	
	set_input_mode();

	struct option longopts[] = {
		{"shell", no_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	int opt_ret;
	while((opt_ret = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
		switch(opt_ret) {
			case 's':
				shell_flag = 1;
				break;
			default:
				fprintf(stderr, "Correct Usage: lab1a [--shell]\n");
				exit(EXIT_FAIL);
				break;
		}
	}


	if (shell_flag == 1) {

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
		if (close(pipe1[0]) == -1) {
			print_error_exit();
		}
		if (close(pipe2[1]) == -1) {
			print_error_exit();
		}
		read_write_shell();
	}
	
	else {
		read_write();
	}

	free(buff);
	exit(EXIT_SUCCESS);
}