#ifndef CLIENT_T_8J71L2J3

#define CLIENT_T_8J71L2J3

#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include "room_t.h"

/* FIXME test size, if possible make it bigger */
#define CLIENT_BUFFER_SIZE 128

struct client_t {
	unsigned cl_id;

	int cl_sock;

	char cl_buff[CLIENT_BUFFER_SIZE]; 
	char *cl_write_p;
	size_t cl_free;

	pthread_mutex_t *cl_buff_locked;

	struct room_t *cl_room;
};

/* client.c */
struct client_t *client_init(void); 
int client_buff_push(struct client_t *, char *, size_t); 

/* inlined */
void client_set_id(struct client_t *, const unsigned);
void client_set_sock(struct client_t *, const int);
void client_buff_clear(struct client_t *);
void client_destroy(struct client_t *);
void client_set_room(struct client_t *, struct room_t *);

int client_pull(struct client_t *, char *, size_t);

#endif /* end of include guard: CLIENT_T_8J71L2J3 */
