/*            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
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



#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>

#include "defobj.h"

obj* allocObj(void) {

	obj* t=malloc(TABLEN*sizeof(obj));

	for(int i=0;i<TABLEN;++i) {
		snprintf(t[i].a,12,"indent_o%d",i+1);
		snprintf(t[i].b,24,"description_o%d",i+1);
		t[i].ii=htonl((i+1)*10+1);
		t[i].jj=htonl((i+1)*10+2);

		/* We cast without implicit binary conversion and then convert to network endianness */
		uint64_t tmp;
		*(double*)&tmp=(double)0.2345+(i+1)*10;
		t[i].dd=htonll(tmp);

		t[i].iqt=0;
	}

	return t;
}

void freeObj(obj* t) {
	free(t);
}

