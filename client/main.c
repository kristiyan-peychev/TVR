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

#include "status.h"

#define PORT "3490" // the port client will be connecting to 

#define PATH_TO_RECORD "/home/kawaguchi/record_bkup/client/record"
#define PATH_TO_PLAYBACK "/usr/bin/aplay"

#define BUFF_SIZE 4096
#define MAXDATASIZE 100 // max number of bytes we can get at once 

void recv_handler(int *sock) 
{
	int i = 0;
	char **ename;

	if (*sock < 0) {
		fprintf(stderr, "Fuck off.\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "Recieving on socket: %d\n", *sock);

	ename = (char **) malloc(4 * sizeof(*ename)); 
	*ename = (char *) malloc((32 * 4) * sizeof(**ename)); 
	while (i < 4) {
		*(ename + i) = *ename + (128 * i);
		++i;
	}
	
	strcpy(*ename, PATH_TO_PLAYBACK);
	strcpy(*(ename + 1), "-f");
	strcpy(*(ename + 2), "cd");
	*(ename + 3) = (char *) 0;
	
	if (fork()) {
		/* ROFAL */
		if (dup2(*sock, STDIN_FILENO) == -1) {
			close(*sock);
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		execvp(*ename, ename);
		perror("dup2");
		exit(EXIT_FAILURE);
	}
}

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
	int sockfd;  
	int rv, i = 0;
	int room_request;
	int send_pipes[2];
	/*char buf[MAXDATASIZE];*/
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];
	char **ename = (char **) malloc(4 * sizeof(*ename)); 
	char buff[BUFF_SIZE];

	if (argc != 3) {
		fprintf(stderr,"usage: client hostname room\n");
		exit(1);
	}

	room_request = atoi(*(argv + 2));

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

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

	freeaddrinfo(servinfo); 

	/*0888318047*/

	/*ename = (char **) malloc(4 * sizeof(char *))*/
	*ename = (char *) malloc((128 * 4) * sizeof(char));
	while (i < 4) {
		*(ename + i) = *ename + (128 * i);
		++i;
	}

	strcpy(*(ename + 0), PATH_TO_RECORD);
	strcpy(*(ename + 1), "-");
	*(ename + 2) = (char *) NULL;

	fprintf(stderr, "send\n");
	if (send(sockfd, &room_request, sizeof(room_request), 0) < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "recv\n");
	if (recv(sockfd, &room_request, sizeof(room_request), 0) < 0) {
		perror("recv");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "Server sent status 0x%x\n", room_request);
	if (room_request != STATUS_SUCCESS) 
		exit(EXIT_SUCCESS);

	if (pipe(send_pipes)) {
		close(sockfd);
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	if (fork()) {
		close(sockfd);
		close(send_pipes[0]);
		if (dup2(send_pipes[1], STDOUT_FILENO) == -1) {
			close(send_pipes[1]);
			close(sockfd);
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		execvp(*ename, ename);
		perror("execvp");
		exit(EXIT_FAILURE);
	} else {
		close(send_pipes[1]);
		/*i = 0;*/

		recv_handler(&sockfd);
		while (read(send_pipes[0], buff, BUFF_SIZE) > 0) 
			send(sockfd, buff, BUFF_SIZE, 0);
		close(sockfd);
	}
	close(sockfd); /* Why... */

	return 0;
}

