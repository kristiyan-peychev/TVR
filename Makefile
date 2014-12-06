CC = gcc
CFLAGS = -c
LDFLAGS = 
LDFLAGS_RECORDER = -lasound

default: arecord.o run_daemon.o 
	$(CC) $(LDFLAGS) run_daemon.o -o run_daemon
	$(CC) $(LDFLAGS) $(LDFLAGS_RECORDER) arecord.o -o record

arecord.o:
	$(CC) $(CFLAGS) arecord.c

run_daemon.o:
	$(CC) $(CFLAGS) run_daemon.c

clean-o:
	rm *.o
clean-wav:
	rm *.wav
