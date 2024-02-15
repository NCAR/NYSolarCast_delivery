#include <stdlib.h>
#include <string.h>
#include "log/log.hh"
#include "emalloc.h"

extern Log *logFile;

/*
 * Check return from malloc() and just exit (with a message) if we're out
 * of memory.
 */
void *
emalloc (size_t size)
{
    void   *p = (void *) malloc (size);
    if (p == 0 && size > 0) {
	logFile->write_time("Error: malloc: out of memory\n");
	exit (1);
    }
    return p;
}


/*
 * Check return from realloc() and just exit (with a message) if we're out
 * of memory.
 */
void *
erealloc (
    void *ptr,
    size_t size
	)
{
    void   *p = realloc (ptr, size);
    if (p == 0 && size > 0) {
	logFile->write_time("Error: realloc: out of memory\n");
	exit (1);
    }
    return p;
}


/*
 * Check return from strdup() and just exit (with a message) if we're out
 * of memory.
 */
char *
estrdup (char *str)
{
    char *s = (char *)emalloc(strlen(str) + 1);
    return strcpy(s, str);
}
