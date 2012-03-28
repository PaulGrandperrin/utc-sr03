HARDENING_CFLAGS=-fPIE -fstack-protector -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security -fPIC
HARDENING_LDFLAGS=-fPIE -pie -Wl,-z,relro -Wl,-z,now

HARDENING_GCC_ONLY_CFLAGS=--param ssp-buffer-size=4
HARDENING_GCC_ONLY_LDFLAGS=


CC=clang
CFLAGS= -O3 -Wall -Wextra -std=c99 -pedantic
LDFLAGS=

CC=clang
CFLAGS= -O3 -Wall -Wextra -std=c99 -pedantic
LDFLAGS= -O4 -Xlinker -plugin=LLVMgold.so -fPIE -pie -Wl,-z,relro -Wl,-z,now

CC = gcc
CFLAGS= -flto -Wall -Wextra -O0 -g -ggdb -p -pg -std=c99 -pedantic -Wstrict-aliasing
LDFLAGS= -flto

CC = gcc
CFLAGS= -flto -Wall -Wextra -O3 -std=c99 -pedantic $(HARDENING_CFLAGS) $(HARDENING_GCC_ONLY_LDFLAGS)
LDFLAGS= -flto $(HARDENING_LDFLAGS) $(HARDENING_GCC_ONLY_LDFLAGS)

all: client server

client: client.c defobj.h initobj.h
	$(CC) $(CFLAGS) $(LDFLAGS) client.c -o client

server: server.c defobj.h
	$(CC) $(CFLAGS) $(LDFLAGS) server.c -o server

clean:
	rm -f server client

