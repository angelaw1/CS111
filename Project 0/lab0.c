// NAME: Angela Wu
// EMAIL: angelawu.123456789@gmail.com
// ID: 604763501

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

void handler(int sig) {
  if (sig == SIGSEGV) {
    fprintf(stderr, "Segmentation Fault: %d\n", sig);
    exit(4);
  }
}

int main(int argc, char **argv) {

  // parse options
    struct option longopts[] =
      { {"input", required_argument, NULL, 'i'},
	{"output", required_argument, NULL, 'o'},
	{"segfault", no_argument, NULL, 's'},
	{"catch", no_argument, NULL, 'c'},
	{0, 0, 0, 0} };

    int opt_ret;

    // used to force segmentation fault
    int segfault = 0;
    char *segfault_ptr = NULL;
    
    int ifd;
    int ofd;
    while((opt_ret = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
      switch(opt_ret) {
      // file redirection
      case 'i':
	if ((ifd = open(optarg, O_RDONLY)) == -1) {
	  fprintf(stderr, "%s\n", strerror(errno));
	  exit(2);
	}
	else {
	  close(0);
	  if (dup2(ifd, 0) == -1) {
	    fprintf(stderr, strerror(errno));
	    exit(-1);
	  }
	  close(ifd);
	}
	break;
      case 'o':
	if ((ofd = creat(optarg, S_IRWXU)) == -1) {
	  fprintf(stderr, "%s\n", strerror(errno));
	  exit(3);
	}
	else {
	  close(1);
	  if (dup2(ofd, 1) == -1) {
	    fprintf(stderr, strerror(errno));
	    exit(-1);
	  }
	  close(ofd);
	}
	break;
      case 's':
	segfault = 1;
	break;
      case 'c':
	// register the signal handler
	signal(SIGSEGV, handler);
	break;
      default:
	fprintf(stderr, "Correct Usage: lab0 --input=inputfile --output=outputfile [segfault catch]\n");
	exit(1);
	break;
      }
    }

    // cause the segfault
    if (segfault == 1) {
      *segfault_ptr = 'A'; \
    }

    // if no segfault was caused, copy stdin to stdout
    char buf[1];
    int ret;
    while ((ret = read(0, buf, 1)) != 0) {
      if (ret < 0) {
	fprintf(stderr, strerror(errno));
	exit(3);
      }
      if (write(1, buf, 1) == -1) {
	fprintf(stderr, strerror(errno));
	exit(3);
      }
    }
    exit(0);
}
