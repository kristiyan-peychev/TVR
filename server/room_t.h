#ifndef ROOM_DX67OSYT

#define ROOM_DX67OSYT

#include <pthread.h>
#include <sys/select.h>
#include "client_t.h"

#define ROOM_MAX_MEMBERS 4 
#define PULL_SZ 128

struct room_t {
	unsigned rm_id;

	struct client_t **rm_members;
	size_t rm_mem_count;
	size_t rm_size; /* of members */
	
	pthread_mutex_t *rm_locked;
	pthread_t rm_thread;

	fd_set rm_sockset;
	int rm_highsocket;
};

/* room.c */

struct room_t *room_init(void);
void room_destroy(struct room_t *);

void room_set_id(struct room_t *, unsigned);
char room_add_member(struct room_t *, struct client_t *);
size_t room_free_space(struct room_t *);
char room_remove_member(struct room_t *, struct client_t *);
/* Arguments: target, buffer, buffer size, sender */
char room_send(struct room_t *, char *, size_t, struct client_t *);

void room_spawn_thread(struct room_t *);

#ifndef ROOM_WRQ 
#define ROOM_WRQ
#define room_wrq(a,b,c,d) room_send(a, b, c, d)
#endif

#endif /* end of include guard: ROOM_DX67OSYT */
