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

libhast.a: hastread.o hashfuncs.o hastwrite.o
	$(AR) rc $@ $^

hast: hastmain.o hastwrite.o hastread.o hashfuncs.o
	$(CC) -o $@ $^

libhast.so: hastread.o hashfuncs.o hastwrite.o
	$(CC) -shared -o $@ $^


hastread.o: CFLAGS:=$(CFLAGS) -fPIC
hastread.o: hastread.c
	$(CC) $(INC) $(CFLAGS) -fPIC -c $<

hastwrite.o: CFLAGS:=$(CFLAGS) -fPIC
hastwrite.o: hastwrite.c
	$(CC) $(INC) $(CFLAGS) -c $<

hashfuncs.o: CFLAGS:=$(CFLAGS) -DSFHASH
hashfuncs.o: hashfuncs.c
	$(CC) $(INC)  $(CFLAGS) -c $<

clean:
	rm -f *.o hast libhast.a libhast.so

all: hast libhast.a libhast.so

again: clean all

ipsrch.o: ipsrch.c
	$(CC) $(INC)  $(CFLAGS) -c $<

ipsrch-main.o: CFLAGS:=$(CFLAGS) -DIPSRCH_MAIN
ipsrch-main.o: ipsrch.c
	$(CC) $(INC)  $(CFLAGS) -o $@ -c $<

ipsrch: ipsrch-main.o hastread.o  hashfuncs.o
	$(CC) -o $@ $^

