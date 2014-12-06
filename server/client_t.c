#include "client_t.h"

inline void client_set_id(struct client_t *target, const unsigned id) 
{
	target->cl_id = id;
}

inline void client_set_sock(struct client_t *target, const int sock) 
{
	target->cl_sock = sock;
}

inline void client_set_room(struct client_t *target, struct room_t *room) 
{
	target->cl_room = room;
}

inline void client_buff_clear(struct client_t *target) 
{ /* Can we do better here? */
	memset(target->cl_buff, 0, CLIENT_BUFFER_SIZE);
	target->cl_free = CLIENT_BUFFER_SIZE;
}

static void client_dump_buffer(struct client_t *target) 
{
	pthread_mutex_lock(target->cl_buff_locked);
	if (send(target->cl_sock, target->cl_buff, CLIENT_BUFFER_SIZE,0) < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}
	memset(target->cl_buff, 0, CLIENT_BUFFER_SIZE);
	target->cl_write_p = target->cl_buff;
	pthread_mutex_unlock(target->cl_buff_locked);
}

int client_buff_push(struct client_t *target, char *buff, size_t sz) 
{ /* TODO: mutex; XXX <- Why is this highlighted? */
	int c = 0;
	pthread_mutex_lock(target->cl_buff_locked);

	while (c < sz && target->cl_free > 0) {
		*target->cl_write_p = *buff++;
		++c;
		--target->cl_free;
		++target->cl_write_p;
	}

	if (target->cl_free == 0) {
		client_dump_buffer(target);
		target->cl_free = CLIENT_BUFFER_SIZE;
	}
	pthread_mutex_unlock(target->cl_buff_locked);

	if (sz > c) 
		return c + client_buff_push(target, buff, sz);
return c;
}

struct client_t *client_init(void) 
{
	struct client_t *ret;
	ret = (struct client_t *) malloc(1 * sizeof(struct client_t)); 

	ret->cl_id = 0;
	ret->cl_sock = 0;
	ret->cl_write_p = ret->cl_buff;
	ret->cl_free = CLIENT_BUFFER_SIZE;
	ret->cl_room = NULL;
	memset(ret->cl_buff, 0, CLIENT_BUFFER_SIZE);
	pthread_mutex_init(ret->cl_buff_locked, NULL);
return ret;
}

/*inline */void client_destroy(struct client_t *target) 
{
	if (target == NULL) 
		return;

	close(target->cl_sock);
	pthread_mutex_destroy(target->cl_buff_locked);
	free(target);
}

/* Write request */
inline void client_wrq(struct client_t *target, char *buff, size_t sz) 
{
	#ifdef ROOM_WRQ
	room_wrq(target->cl_room, buff, sz, target);
	#else 
	room_add_buff(target->cl_room, buff, sz, target);
	#endif
}

inline int client_pull(struct client_t *target, char *buff, size_t sz) 
{
	return recv(target->cl_sock, buff, sz, 0);
}

