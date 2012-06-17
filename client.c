/*            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2012 Paul Grandperrin <paul.grandperrin@gmail.com>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 */



//#define _POSIX_SOURCE
//#define _XOPEN_SOURCE 500
//#define _BSD_SOURCE
#define _GNU_SOURCE /* includes most non-conflicting standards */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> /* memset */

#include <errno.h>
#include <netdb.h> /* getnameinfo & getaddrinfo */

#include <sys/socket.h>
#include <sys/types.h> /* historical BSD compatibility */

/* include our architecture independant object structure */
#include "defobj.h"

/* include function to allocate and free the object */
#include "initobj.h"


int main ( int argc, char* argv[] ) {

	if (argc != 3) {
		fprintf(stderr,
				"Incorrect number of arguments\n\
				Usage: client name/address service/port\n");
		goto exit_failure;
	}

	/* Connection socket */
	int sock;

	/* error code returned by function calls */
	int err;

	/**
	 * Open TCP/IPv6 socket
	 */
	struct addrinfo hints;

	memset(&hints, 0, sizeof hints); /* bzero is deprecated */

	/** we specify our socket contraints */
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags= 0;
	hints.ai_protocol=IPPROTO_TCP; /* not really mandatory */

	/**
	 * Get a linked list of socket configurations which
	 * satisfy our constraints.
	 */
	struct addrinfo *result, *rp;
	err=getaddrinfo(argv[1],argv[2], &hints, &result);
	if(err) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		goto exit_failure;
	}

	/** We test each socket configuration until one succeed */
	for(rp=result; rp!=NULL;rp=rp->ai_next) {

		sock=socket ( rp->ai_family,rp->ai_socktype,rp->ai_protocol );
		if ( sock==-1 ) {
			perror("socket");
			continue;
		}

		err=connect(sock, rp->ai_addr, rp->ai_addrlen);
		if(err) {
			perror("socket");
			close(sock);
			continue;
		}

		/* we connected successfully */
		break;
	}
	/* free the result linked list allocated by the
	 * getaddrinfo function call.
	 */
	freeaddrinfo(result);

	/* we didn't connected successfully */
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		goto exit_failure;
	}

	/**
	 * FIXME
	 * RANDOM SEND AND RECEIVE STUFF FOR THE MOMENT
	 */

	obj* tabObj=allocObj();
	for(int i=0; i<(TABLEN+1); ++i) {
		tabObj[i].iqt=(i==TABLEN)?-1:0;
		send(sock,(void *) &tabObj[i],sizeof(obj),0);
	}
	freeObj(tabObj);

	char a[2];
	recv(sock,a,1,0);
	a[1]=0;
	printf("%s\n",a);

	close(sock);

	return EXIT_SUCCESS;

	exit_failure:
	return EXIT_FAILURE;
}
