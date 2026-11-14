CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=gnu11

all: parent childe1 childe2

parent: parent.c
	$(CC) $(CFLAGS) -o $@ $<

childe1: childe1.c
	$(CC) $(CFLAGS) -o $@ $<

childe2: childe2.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f parent childe1 childe2
