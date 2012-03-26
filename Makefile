CC=clang
CFLAGS= -O3 -Wall -Wextra -std=c99 -pedantic
LDFLAGS=

CC = gcc
CFLAGS= -flto -Wall -Wextra -O0 -g -ggdb -p -pg -std=c99 -pedantic -Wstrict-aliasing
LDFLAGS= -flto

CC=clang
CFLAGS= -O4 -Wall -Wextra -std=c99 -pedantic
LDFLAGS= -O4 -Xlinker -plugin=LLVMgold.so

all: client server

client: client.c defobj.h initobj.h
	$(CC) $(CFLAGS) $(LDFLAGS) client.c -o client

server: server.c defobj.h
	$(CC) $(CFLAGS) $(LDFLAGS) server.c -o server

clean:
	rm -f server client

