# macros definitions
CC = gcc
CFLAGS = -Wall
XHDRS = headers.h

bckp: bckp.o common.o rstr
	$(CC) $(CFLAGS) bckp.o common.o -o $@

rstr: rstr.o common.o
	$(CC) $(CFLAGS) rstr.o common.o -o $@

#common.o: common.c common.h headers.h
#	$(CC) $(CFLAGS) -c common.c

#bckp.o: bckp.c bckp.h headers.h
#	$(CC) $(CFLAGS) -c bckp.c

#rstr.o: rstr.c rstr.h headers.h
#	$(CC) $(CFLAGS) -c rstr.c

%.o: %.c %.h $(XHDRS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *~