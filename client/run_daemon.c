#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

int daemonize_no_touch_fdz(void)
{
	pid_t pid;
	pid = fork();
	if (pid < 0)
		return -1;
	if (pid > 0)
		_exit(0);
	if ( setsid() < 0 )
		return -1;

	return 0;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Please specify a command to execute\n");
		return 1;
	}
	daemonize_no_touch_fdz(); /* Daemonize the process so it runs in the background */
	pid_t ffdf;
	int f = 0;
	int pipes[2];
	char buff[1028];
	if (pipe(pipes)) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	char **args = (char **) malloc((argc - 1) * sizeof(char *)); 
	while (f < argc - 1) { /* Setting up the new executable's arguments */
		*(args + f) = *(argv + f + 1);
		++f;
	}
	*(args + f) = (char *) 0;

	ffdf = fork();
	if (!ffdf) { /* Child */
		close(pipes[0]);
		dup2(STDOUT_FILENO, pipes[1]);
		execvp(*args, args); /* Execute the command, we're leaking memory btw */
		perror("execvp"); /* This part will execute if the execute should fail */
		exit(EXIT_FAILURE);
	} else { /* Parent */
		close(pipes[1]);
		while (read(pipes[0], buff, 1024) == 1024)  // FIXME
			//fprintf(stdout, "%1024c", buff);
			write(STDOUT_FILENO, buff, 1024);
		//kill(ffdf, SIGTERM);
	}
return 0;
}

