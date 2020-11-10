TEAM = NOBODY
VERSION = 1
HANDINDIR = /afs/cs/academic/class/15213-f02/L7/handin

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

OBJS = proxy.o csapp.o

all: proxy

proxy: $(OBJS)
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

handin:
	cp proxy.c $(HANDINDIR)/$(TEAM)-$(VERSION)-proxy.c

clean:
	rm -f *~ *.o proxy core

