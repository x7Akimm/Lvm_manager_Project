CC = gcc
CFLAGS = -Wall -O2 -pthread
LDFLAGS = -pthread

all: setup supervisor

# setup init
setup: setup.o config.o
	$(CC) $(CFLAGS) -o setup setup.o config.o $(LDFLAGS)

# build supervisor thread
supervisor: supervisor.o balancer.o writer.o config.o
	$(CC) $(CFLAGS) -o supervisor supervisor.o balancer.o writer.o config.o $(LDFLAGS)

# compile
setup.o: setup.c config.h
	$(CC) $(CFLAGS) -c setup.c

supervisor.o: supervisor.c balancer.h writer.h config.h
	$(CC) $(CFLAGS) -c supervisor.c

balancer.o: balancer.c balancer.h config.h
	$(CC) $(CFLAGS) -c balancer.c

writer.o: writer.c writer.h config.h
	$(CC) $(CFLAGS) -c writer.c

config.o: config.c config.h
	$(CC) $(CFLAGS) -c config.c

# clean build
clean:
	rm -f *.o setup supervisor
