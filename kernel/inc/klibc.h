
#ifndef __K_STRING_H__
#define __K_STRING_H__


#ifdef __cplusplus
// using size_t = long;
extern "C"{
#endif

#include <sbi/sbi_types.h>
/*
  Provides strcmp for the completeness of supporting string functions.
  it is not recommended to use strcmp() but use strncmp instead.
*/

int strcmp(const char *a, const char *b);

int strncmp(const char *a, const char *b, size_t count);

size_t strlen(const char *str);

size_t strnlen(const char *str, size_t count);

char *strcpy(char *dest, const char *src);

char *strncpy(char *dest, const char *src, size_t count);

char *strchr(const char *s, int c);

char *strrchr(const char *s, int c);

void *memset(void *s, int c, size_t count);

void *memcpy(void *dest, const void *src, size_t count);

void *memmove(void *dest, const void *src, size_t count);

int memcmp(const void *s1, const void *s2, size_t count);

void *memchr(const void *s, int c, size_t count);

void int2str(int num,char* buffer);

#ifdef __cplusplus
}
#endif

#endif
