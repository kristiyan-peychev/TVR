#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define BUFF_SIZE 16
#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	int i = 0;

	if (argc != 2) {
		fprintf(stderr,"usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	/*
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);
	*/
	char **ename = (char **) malloc(4 * sizeof(char *));
	char buff[BUFF_SIZE];
	pid_t ffdf;
	/*
	while (i < 4) {
		*(ename + i) = (char *) malloc(128 * sizeof(char));
		++i;
	} 0888318047
	*/
	*ename = (char *) malloc((128 * 4) * sizeof(char));
	while (i < 4) {
		*(ename + i) = *ename + (128 * i);
		++i;
	}
	//strcpy(*ename, "/home/kawaguchi/system-c/week4/record/client/run");
	strcpy(*(ename + 0), "/home/kawaguchi/system-c/week4/record/client/record"); /* FIXME */
	strcpy(*(ename + 1), "-");
	*(ename + 2) = (char *) NULL;
	int pipes[2];
	if (pipe(pipes)) {
		close(sockfd);
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	ffdf = fork();
	if (!ffdf) {
		//close(sockfd);
		close(pipes[0]);
		if (dup2(pipes[1], STDOUT_FILENO) == -1) {
			close(pipes[1]);
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		execvp(*ename, ename);
		perror("execvp");
		exit(EXIT_FAILURE);
	} else {
		close(pipes[1]);
		i = 0;
		while (read(pipes[0], buff, BUFF_SIZE) > 0) 
			send(sockfd, buff, BUFF_SIZE, 0);
		close(sockfd);
	}
	close(sockfd);

	return 0;
}

