#ifndef STDLIB_D
#define STDLIB_D

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

typedef long unsigned int size_t;
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

typedef unsigned int* va_list;

#define va_start(list,argbefore) list = (void*)&(argbefore)
#define va_arg(list,type) (*(type*)(++(list)))
#define va_end(list)

extern int write(int file, const void *buffer, size_t count);
extern size_t read(int fd, void *buffer, size_t length);

void memcpy(void *dest, void *src, size_t n);
int puts(const char *string);
void putchar(char c);
char *convert(unsigned int num, int base);
void printf(char* format,...);
char *getline(void);

#endif
