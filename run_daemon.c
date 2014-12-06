#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Please specify a command to execute\n");
		return 1;
	}
	//daemon(0, 0); /* Daemonise the process so it runs in the background */
	pid_t ffdf;
	int f = 0;
	char **args = (char **) malloc((argc - 1) * sizeof(char *)); 
	while (f < argc - 1) {
		*(args + f) = *(argv + f + 1);
		++f;
	}
	*(args + f) = (char *) 0;
	ffdf = fork();
	if (!ffdf) { /* Child */
		execvp(*args, args); /* Execute the command, we're leaking memory btw */
		perror("execvp"); /* This part will execute if the execute should fail */
		exit(EXIT_FAILURE);
	} else { /* Parent */
		usleep(1000000);
		kill(ffdf, SIGTERM);
	}
return 0;
}

