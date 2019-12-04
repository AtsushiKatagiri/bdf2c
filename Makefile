CC=gcc
CFLAGS=-Wall -O2

all: bdf2c

bdf2c: bdf2c.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) bdf2c
