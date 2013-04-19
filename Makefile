# macros definitions
CC = gcc
CFLAGS = -Wall
XHDRS = headers.h

all: bckp rstr

bckp: bckp.o common.o
	$(CC) $(CFLAGS) bckp.o common.o -o $@

rstr: rstr.o common.o
	$(CC) $(CFLAGS) rstr.o common.o -o $@

%.o: %.c %.h $(XHDRS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
