#ifndef ALLOC_D
#define ALLOC_D

#include "stdlib.h"
#include "sbrk.h"

/* titel: malloc()/free()-Paar nach K&R 2, p.185ff */

#define NALLOC 	1024	/* Mindestgroesse fuer morecore()-Aufruf	*/

typedef long Align;

union header {			/* Kopf eines Allokationsblocks */
    struct {
	union header	*ptr;  	/* Zeiger auf zirkulaeren Nachfolger */
	unsigned int 	size;	/* Groesse des Blocks	*/
    } s;
    Align x;			/* Erzwingt Block-Alignierung	*/
};

typedef union header Header;

static Header base;		/* Anfangs-Header	*/
static Header *freep = NULL;	/* Aktueller Einstiegspunkt in Free-Liste */

void *realloc(void * ptr, size_t size);

void* malloc(size_t nbytes);

/* Eine static-Funktion ist ausserhalb ihres Files nicht sichtbar	*/

void free(void *ap);

static Header *morecore(size_t nu);

#endif
