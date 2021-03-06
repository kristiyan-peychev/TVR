#include "client_t.h"
#include "room_t.h"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>


inline void room_set_id(struct room_t *target, unsigned id) 
{
	target->rm_id = id;
}

inline size_t room_mem_count(struct room_t *target) 
{
	return target->rm_mem_count;
}

static size_t check_if_member(struct room_t *target, struct client_t *victim) 
{ /* TODO optimizations? */
	size_t i = 0;

	while (i < target->rm_mem_count) {
		if ((*(target->rm_members + i))->cl_id == victim->cl_id) 
			return i;
		++i;
	}
	return 0;
}

void insert_where(struct room_t *target, struct client_t *victim) 
{
	target->rm_members[target->rm_mem_count++] = victim;
}

char room_add_member(struct room_t *target, struct client_t *victim) 
{
	/*int index;*/
	fprintf(stderr, "Adding member in room #%d\n", target->rm_id);

	if (target->rm_mem_count >= ROOM_MAX_MEMBERS) 
		return 1;

	/*pthread_mutex_lock(&target->rm_locked);*/
	if (target->rm_members == NULL) {
		/* 
		 * This will probably be left unused, but is here 
		 * for precausion, in case somebody touches anything 
		 */
		target->rm_size = 4;
		target->rm_members = (struct client_t **) malloc(
				target->rm_size * sizeof(struct client_t *)); 
	} else if (target->rm_size == target->rm_mem_count) {
		target->rm_size *= 2;
		target->rm_members = (struct client_t **) realloc( 
					target->rm_members, 
					target->rm_size);
	}
	
	if (!check_if_member(target, victim)) {
		/**(target->rm_members + target->rm_mem_count++) = victim;*/

		/* FIXME!!!! */
		insert_where(target, victim);
		fprintf(stderr, "Success!\n");
		/*pthread_mutex_unlock(&target->rm_locked);*/
		return 0;
	}
	/*pthread_mutex_unlock(&target->rm_locked);*/
	return 1;
}

static void room_remove_sth(struct room_t *target, size_t s) 
{
	target->rm_members[s]->cl_room = NULL;

	--target->rm_mem_count;
	while (s < target->rm_mem_count) {
		target->rm_members[s] = target->rm_members[s + 1];
		++s;
	}
}

char room_remove_member(struct room_t *target, struct client_t *victim)
{
	size_t s;

	if ((s = check_if_member(target, victim))) 
		return 1;

	pthread_mutex_lock(&target->rm_locked);
	room_remove_sth(target, s);
	pthread_mutex_unlock(&target->rm_locked);
	return 0;
}

inline size_t room_free_space(struct room_t *target)
{
	return ROOM_MAX_MEMBERS - target->rm_mem_count;
}

struct room_t *room_init(void)
{
	struct room_t *ret;
	ret = (struct room_t *) malloc(1 * sizeof(struct room_t));
	ret->rm_id = 0;
	ret->rm_size = 4;
	ret->rm_members = 0;
	ret->rm_highsocket = 0;
	FD_ZERO(&ret->rm_sockset);
	ret->rm_members = (struct client_t **) 
			malloc(4 * sizeof(struct client_t *));
	pthread_mutex_init(&ret->rm_locked, NULL);
	return ret;
}

void room_destroy(struct room_t *target) 
{
	size_t i = 0;
	
	if (target == NULL) 
		return;

	while (i < target->rm_mem_count) 
		client_destroy(target->rm_members[i++]);
	free(target->rm_members);

	pthread_mutex_destroy(&target->rm_locked);
}

/* Should this be static, or what? */
char room_send(struct room_t *target, char *buff, size_t sz,
		struct client_t *sender) 
{ 
	size_t i = 0;
	char ret;

	/*fprintf(stderr, "Sending some shit to %d\n", target->rm_mem_count);*/
	pthread_mutex_lock(&target->rm_locked);

	while (i < target->rm_mem_count) {
		/*fprintf(stderr, "Send\n");*/
		if (target->rm_members[i] == sender) {
			++i;
			continue;
		}
		ret = (client_buff_push(target->rm_members[i++], buff, sz) > 0 ?
				1 : 0);
		++i;
	}

	pthread_mutex_unlock(&target->rm_locked);
	/*fprintf(stderr, "Ret\n");*/
	return ret;
}

static void room_build_select_list(struct room_t *target)
{
	unsigned i = 0;
	FD_ZERO(&target->rm_sockset);

	while (i < target->rm_mem_count) {
		if (target->rm_members[i]->cl_sock != 0) {
			/*fprintf(stderr, "Add\n");*/
			FD_SET(target->rm_members[i]->cl_sock, 
					&target->rm_sockset);

			if (target->rm_members[i]->cl_sock > 
			    target->rm_highsocket) 
				target->rm_highsocket = 
					target->rm_members[i]->cl_sock;
		}
		++i;
	}
}

static void room_handle_data(struct room_t *target)
{
	size_t i = 0;
	int sz;
	char buff[CLIENT_BUFFER_SIZE];
	/*fprintf(stderr, "Handling data\n");*/

	while (i < target->rm_mem_count) {
		/* FIXME macro this damn shit? */
		if (FD_ISSET(target->rm_members[i]->cl_sock, 
				&target->rm_sockset)) 
		{
			/* FIXME this might be hella wrong */
			if ((sz = client_pull(target->rm_members[i],
					buff, PULL_SZ)) < 0) 
			{
				client_dc(target->rm_members[i]);
				perror("recv");
				continue;
			} else {
				room_send(target, buff, sz, 
					target->rm_members[i]);
			}
		}
		++i;
	}
}

/*static char room_main_loop(struct room_t *target) */
static void *room_main_loop(void *tar_dummy)
{
	struct room_t *target = (struct room_t *) tar_dummy;
	struct timeval timeout;
	int readsocks = 0;

	fprintf(stderr, "Room thread spawned\n");

	do {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		room_build_select_list(target);

		fprintf(stderr, "Room %u is idle\n", target->rm_id);
		readsocks = select(target->rm_highsocket + 1, 
			&target->rm_sockset, NULL, NULL, &timeout);

		if (readsocks < 0) {
			/* FIXME lol */
			perror("select");
			return NULL;
		} else if (readsocks == 0) {
		} else {
			/* HERE'S SOME DATA, HANDLE IT! */
			room_handle_data(target);
		}
	} while (1);

	fprintf(stderr, "Ret??\n");
	/* because fuck you, that's why */
	return tar_dummy;
}

inline void room_spawn_thread(struct room_t *target)
{ 
	pthread_create(&target->rm_thread, NULL, room_main_loop, 
			(void *) target);
}

