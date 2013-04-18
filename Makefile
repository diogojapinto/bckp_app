# macros definitions
CC = gcc
CFLAGS = -Wall
HDRS = libraries.h


bckp: bckp.o
	$(CC) $(CFLAGS) bckp.o -o $@

rstr: rstr.o
	$(CC) $(CFLAGS) rstr.o -o $@

%.o: %.c %.h $(HDRS)
	$(CC) $(CFLAGS) -c $<