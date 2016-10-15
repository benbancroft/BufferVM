#ifndef STDLIB_H
#define STDLIB_H

#define NULL 0

#ifndef EOF
# define EOF (-1)
#endif

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef uint64_t size_t;
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

typedef __builtin_va_list va_list;
typedef __builtin_va_list __isoc_va_list; /* wchar.h */

#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define va_copy(d,s) __builtin_va_copy(d,s)

extern int write(int file, const void *buffer, size_t count);
extern size_t read(int fd, void *buffer, size_t length);

int fgetc(int fd);
void memcpy(void *dest, void *src, size_t n);
void *memset(void *dest, int src, size_t n);
size_t strlen(const char *str);
int puts(const char *string);
void putchar(char c);
char *convert(uint64_t num, size_t base);
void printf(char* format,...);
char *getline(void);

#endif