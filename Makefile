
CC = gcc
CCFLAGS = -g -std=c89 -Wall -Werror -pedantic
# CCFLAGS = -g -std=c89 -Wall -pedantic

all: testhash

test: all
	@./testhash

clean:
	rm *.o

testhash: testhash.c hash.o
	$(CC) $(CCFLAGS) testhash.c hash.o -o testhash

hash.o: hash.c
	$(CC) $(CCFLAGS) -c hash.c -o hash.o



