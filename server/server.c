#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>

#include "client_t.h"
#include "room_t.h"
#include "status.h"

/* I'm not gonna bother commenting on what each include does */ 
/* so tough luck if you were expecting anything of the sort. */ 


#define PORT "3490"  /* the port users will be connecting to */
#define BACKLOG 10   /* how many pending connections queue will hold */
#define BUF_SIZE CLIENT_BUFFER_SIZE

static struct client_t *clients[BACKLOG];
static size_t clients_connected;
static unsigned _cl_ids;


#define ROOM_COUNT 2
#define SERVER_THREAD_COUNT 1
/* The server will hold a thread for each room */
/*static pthread_t rooms[ROOM_COUNT + SERVER_THREAD_COUNT];*/

static struct room_t *rooms[ROOM_COUNT + SERVER_THREAD_COUNT];
static unsigned last_room; /* index of the last free room */


static fd_set sockets; /* Used for select */


void build_select_list(int *sockfd, int *highsock)
{ /* Build the fd_set sockets structure */
	unsigned i = 0;
	FD_ZERO(&sockets);
	
	FD_SET(*sockfd, &sockets);
	while (i < BACKLOG) {
		/* Get all sockets which are not already in a room */
		/* sockets in a room should be handled by their own threads */
		if ((clients[i])->cl_sock != 0 && clients[i]->cl_room == NULL) {
			FD_SET((clients[i])->cl_sock, &sockets);
			if ((clients[i])->cl_sock > *highsock) 
				*highsock = (clients[i])->cl_sock;
		}
		++i;
	}
}

static inline unsigned next_id(void) 
{
	return _cl_ids++;
}

static size_t last_client(void) 
{
	size_t ret = 0;
	while (ret < BACKLOG) {
		if (clients[ret] == NULL) 
			return ret;
		++ret;
	}
return -1;
}

static inline void init_clients(void) 
{ /* Useful only if the clients array is not in the bss sector */
  /* so there is no point in anyone using this right now */
	unsigned i = 0;
	while (i < BACKLOG) 
		clients[i++] = NULL;
		/*clients[i++] = client_init();*/
}

static void init_rooms(void) 
{
	unsigned i = 0;
	while (i < ROOM_COUNT + SERVER_THREAD_COUNT) 
		rooms[i++] = room_init();
}

static void destroy_clients(void) 
{
	unsigned i = 0;
	while (i < BACKLOG) 
		client_destroy(clients[i++]);
}

static void destroy_rooms(void) 
{
	unsigned i = 0;
	while (i < ROOM_COUNT + SERVER_THREAD_COUNT) 
		room_destroy(rooms[i++]);
}

void set_nonblocking(int *sock) 
{ 
	int opts;

	opts = fcntl(*sock,F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		exit(EXIT_FAILURE);
	}

	opts = (opts | O_NONBLOCK);
	if (fcntl(*sock,F_SETFL,opts) < 0) {
		perror("fcntl(F_SETFL)");
		exit(EXIT_FAILURE);
	}
}

/* See if there is any space in the requested room */
static int request_room(int request) 
{ 	if (request < 0 || request > ROOM_COUNT) 
		return -1;
	return room_free_space(rooms[request]);
}

static void handle_new_connection(int *sockfd) 
{
	int connection;
	char status;
	int room_request;
	size_t client_index;

	/* FIXME: Save the IP somewhere, somehow */
	/* or pass it to room_add_member */
	connection = accept(*sockfd, NULL, NULL); 
	
	if (connection < 0) {
		perror("accept");
		/* FIXME? */
		exit(EXIT_FAILURE);
	} else if (clients_connected == BACKLOG) {
		/* Error out, server is full */
		status = STATUS_SERVER_FULL;
		send(connection, &status, sizeof(status), 0);
		close(connection);
	} else if (
		(recv(connection, &room_request, sizeof(room_request), 0) > 0) 
		&& (request_room(room_request) < 0) 
		)
	{ /* Error out, requested room is full */
		status = STATUS_ROOM_FULL;
		send(connection, &status, sizeof(status), 0);
		close(connection);
	} else { /* Everything should be fine, connecting client to room */
		client_index = last_client();
		clients[client_index] = client_init();
		client_set_id(clients[client_index], next_id());
		client_set_sock(clients[client_index], connection);
		client_buff_clear(clients[client_index]);

		if (room_add_member(rooms[room_request], 
				clients[client_index])) 
		{ /* Room is full after all */
			client_destroy(clients[client_index]);
			status = STATUS_ROOM_FULL;
			send(connection, &status, sizeof(status), 0);
			close(connection);
		}

		/*++last_client; [> FIXME thread-safe or atomic; Is this useless? <]*/
	}
}

void read_them_socks(int *sock) 
{
	size_t i = 0;

	if (FD_ISSET(*sock, &sockets)) 
		handle_new_connection(sock);

	while (i < BACKLOG) {
		if (FD_ISSET(clients[i]->cl_sock, &sockets) && clients[i]->cl_room == NULL) {
			/* 
			 * DEAL WITH THE DAMN DATA HERE,
			 * actually if we get here something went wrong 
			 */
			fprintf(stderr, "Error: What?\n");
		}
		++i;
	}
}

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)
{ /* get sockaddr, IPv4 or IPv6: */

	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd; 
	int highsock/*, *socks*/;
	struct addrinfo hints, *servinfo, *p;
	/*struct sockaddr_storage their_addr; [> connector's address information <]*/
	struct sigaction sa;
	/*socklen_t sin_size;*/
	int yes = 1;
	/*char s[INET6_ADDRSTRLEN];*/
	/*char buf[BUF_SIZE];*/
	int rv, readsocks;
	struct timeval timeout;
	

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; /* Either IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP */
	hints.ai_flags = AI_PASSIVE; /* use my IP */

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
					sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}
	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	/* reap all dead processes | LEGACY */
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	set_nonblocking(&sockfd);
	highsock = sockfd;

	init_clients();
	do {
		build_select_list(&sockfd, &highsock);

		timeout.tv_sec = 2; /* Setting timeout to 2 seconds */
		timeout.tv_usec = 0; /* and 0 microseconds */

		readsocks = select(highsock + 1, &sockets, (fd_set *) 0, 
				(fd_set *) 0, &timeout);

		if (readsocks < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		} else if (readsocks == 0) {
			printf("Nothing connected yet, just reporting\n");
		} else {
			/*handle_new_connection(&sockfd);*/
			printf("HOLY FUCKING SHIT, SOMEBODY IS CONNECTING\n");
			read_them_socks(&highsock);
		}
	} while (1);

	#if 0
	while(1) {  /* main accept() loop */
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { /* child */
			close(sockfd); /* child doesn't need the listener */
			/*if (send(new_fd, "Hello, world!", 13, 0) == -1) // This here does all the work
				perror("send");
			*/
			while (recv(new_fd, buf, BUF_SIZE, 0) != -1) 
				write(STDOUT_FILENO, buf, BUF_SIZE);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  /* parent doesn't need this */
	}
	#endif
	
return 0;
}

