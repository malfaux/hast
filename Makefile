CC=cc
INC=-I.
CFLAGS=-Wall -g
LDFLAGS=
OBJS=*.o
AR=ar

hastwrite: hastwrite.o hashfuncs.o
	$(CC) -o $@ $^

hastread: hastread-prog.o hashfuncs.o
	$(CC) -o $@ $^

libhastr.a: hastread.o hashfuncs.o
	$(AR) rc $@ $^

hast: hastmain.o hastwrite.o hastread.o
	$(CC) -o $@ $^

libhastr.so: hastread.o hashfuncs.o
	$(CC) -shared -o $@ $^

hastread-prog.o: CFLAGS:=$(CFLAGS) -DHASTREAD_MAIN
hastread-prog.o: hastread.c
	$(CC) $(INC) $(CFLAGS) -o $@ -c $<

hastread.o: hastread.c
	$(CC) $(INC) $(CFLAGS) -fPIC -c $<

hastwrite.o: CFLAGS:=$(CFLAGS) -DHASTPROG
hastwrite.o: hastwrite.c
	$(CC) $(INC) $(CFLAGS) -c $<

hashfuncs.o: CFLAGS:=$(CFLAGS) -DSFHASH
hashfuncs.o: hashfuncs.c
	$(CC) $(INC)  $(CFLAGS) -c $<

clean:
	rm -f *.o hastwrite hastread ipsrch

all: hastread hastwrite ipsrch

again: clean all

ipsrch.o: ipsrch.c
	$(CC) $(INC)  $(CFLAGS) -c $<

ipsrch-main.o: CFLAGS:=$(CFLAGS) -DIPSRCH_MAIN
ipsrch-main.o: ipsrch.c
	$(CC) $(INC)  $(CFLAGS) -o $@ -c $<

ipsrch: ipsrch-main.o hastread.o  hashfuncs.o
	$(CC) -o $@ $^

