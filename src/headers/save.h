#ifndef _SL_INCLUDED
#define _SL_INCLUDED

#include "soul_create.h"

#define PASSWD 0	// 1 = passwords enabled.

int   file_exists(char *ptr);
int   save(soul_t *ptr);
int   load(soul_t *ptr);
void  ntof(char *filename);
float load_d(int code);
unsigned long hash(unsigned char *plaintext);

#endif