#ifndef STDLIB_H
#define STDLIB_H

#define NULL 0

#define UINT64_C(c)	c ## UL

#ifndef EOF
# define EOF (-1)
#endif

#define	false	0
#define	true	1
#define	bool	_Bool

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef uint64_t size_t;
typedef int64_t ssize_t;
typedef int64_t intptr_t;
typedef uint64_t *uintptr_t;

typedef __builtin_va_list va_list;
typedef __builtin_va_list __isoc_va_list; /* wchar.h */

#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define va_copy(d,s) __builtin_va_copy(d,s)

extern ssize_t write(int file, const void *buffer, size_t count);
extern ssize_t read(int fd, void *buffer, size_t length);

int fgetc(int fd);
void memcpy(void *dest, void *src, size_t n);
void *memset(void *dest, int src, size_t n);
size_t strlen(const char *str);
int puts(const char *string);
void putchar(char c);
char *convert(uint64_t num, size_t base);
void printf(char* format,...);
char *getline(void);

void *set_version_ptr(uint64_t version, uint64_t *pointer);
uint64_t get_version_ptr(uint64_t *pointer);

#endif
