#ifndef __STRING_H__
#define __STRING_H__

#define MAX_STRING_LEN 256

// string stuff
void strncpy(char *dest, const char *src, long len);

int strlen(const char *str);

void *memcpy(void *dest, const void *src, long n);

#endif // __STRING_H__
