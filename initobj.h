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



#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>

#include "defobj.h"

obj* allocObj(void) {

	obj* t=malloc((TABLEN+1)*sizeof(obj));

	snprintf(t[0].a,12," ");
	snprintf(t[0].b,24," ");
	t[0].ii=htonl(TABLEN);
	t[0].jj=htonl(0);
	t[0].dd=0;

	t[0].iqt=0;

	for(int i=1;i<(TABLEN+1);++i) {
		snprintf(t[i].a,12,"indent_o%d",i+1);
		snprintf(t[i].b,24,"description_o%d",i+1);
		t[i].ii=htonl((i+1)*10+1);
		t[i].jj=htonl((i+1)*10+2);

		/* We cast without implicit binary conversion and then convert to network endianness */
		union double2uint64 tmp;
		tmp.d=(double)0.2345+(i+1)*10;
		t[i].dd=htonll(tmp.u);

		t[i].iqt=0;
	}

	return t;
}

void freeObj(obj* t) {
	free(t);
}

