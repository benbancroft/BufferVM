#include "stdlib.h"

void memcpy(void *dest, void *src, size_t n) {
    //Casts
    char *csrc = (char *) src;
    char *cdest = (char *) dest;

    //copy src to dest
    for (int i = 0; i < n; i++)
        cdest[i] = csrc[i];
}

void *memset(void *dest, int c, size_t n)
{
    if (n != 0) {
        char *sp = dest;
        do {
            *sp++ = (char)c;
        } while (--n != 0);
    }
    return (dest);
}

int memcmp(const void *s1, const void *s2, size_t n) {
    size_t i;
    const unsigned char *cs = (const unsigned char*)s1;
    const unsigned char *ct = (const unsigned char*)s2;

    for (i = 0; i < n; i++, cs++, ct++)
    {
        if (*cs < *ct)
        {
            return -1;
        }
        else if (*cs > *ct)
        {
            return 1;
        }
    }
    return 0;
}

int strcmp (const char *s1, const char *s2) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (*p1 != '\0') {
        if (*p2 == '\0') return  1;
        if (*p2 > *p1)   return -1;
        if (*p1 > *p2)   return  1;

        p1++;
        p2++;
    }

    if (*p2 != '\0') return -1;

    return 0;
}

size_t strlen(const char *str)
{
    if (!str) {
        return 0;
    }

    char *ptr = str;
    while (*str) {
        ++str;
    }

    return str - ptr;
}

char *strncat(char *s1, char *s2, int n)
{
    char *os1 = s1;

    while (*s1++);
    --s1;
    while (*s1++ = *s2++)
        if (--n < 0) {
            *--s1 = '\0';
            break;
        }
    return (os1);
}

int puts(const char *str) {
    int i = 0;
    while (str[i++]);
    write(1, str, i);

    return 1;
}

void putchar(char c) {
    (void) write(1, &c, 1);
}

char *convert(int64_t num, size_t base) {
    static const char num_chars[] = "0123456789ABCDEF";
    static char buffer[50];
    char *ptr;

    ptr = &buffer[49];
    *ptr = '\0';

    do {
        *--ptr = num_chars[num % base];
        num /= base;
    } while (num != 0);

    return (ptr);
}


void printf(char *format, ...) {
    char *traverse, *start;
    int i = 0;
    char *s;

    va_list arg;
    va_start(arg, format);

    for (start = traverse = format; *traverse != '\0'; traverse++) {
        if (*traverse != '%') {
            i++;
        } else {
            write(1, start, i);
            traverse++;

            switch (*traverse) {
                case 'c' :
                    i = va_arg(arg, int);     //char argument
                    putchar(i);
                    break;

                case 'd' :
                    i = va_arg(arg, int);         //decimal argument
                    if (i < 0) {
                        i = -i;
                        putchar('-');
                    }
                    puts(convert(i, 10));
                    break;

                case 'o':
                    i = va_arg(arg, unsigned int); //octal
                    puts(convert(i, 8));
                    break;
                case 's':
                    s = va_arg(arg, char *);       //string
                    puts(s);
                    break;
                case 'x':
                    i = va_arg(arg, uint64_t); //hexadecimal
                    puts(convert(i, 16));
                    break;
                case 'p':
                    i = va_arg(arg, uint64_t); //pointer
                    puts("0x");
                    puts(convert(i, 16));
                    break;
            }
            start = traverse + 1;
            i = 0;
        }
    }
    write(1, start, i);

    va_end(arg);
}

int fgetc(int fd) {
    unsigned char ch;
    read(fd, &ch, 1);
    return ch;
}

char *getline(void) {
    char *line = malloc(100), *linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;

    if (line == NULL)
        return NULL;

    for (;;) {
        c = fgetc(0);
        if (c == '\n')
            if (--len == 0) {
                len = lenmax;
                char *linen = realloc(linep, lenmax *= 2);

                if (linen == NULL) {
                    free(linep);
                    return NULL;
                }
                line = linen + (line - linep);
                linep = linen;
            }

        if ((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}

//version stuff

#define VER_NUM_BITS 8
#define VER_NUM_BITMASK (1 << VER_NUM_BITS) - 1

void *set_version_ptr(uint64_t version, void *pointer) {
    return (void *)((uint64_t) pointer | ((version & 0xFF) << (48 - VER_NUM_BITS)));
}

uint64_t get_version_ptr(uint64_t *pointer) {
    return (uint64_t)pointer >> (48 - VER_NUM_BITS);
}

void *normalise_version_ptr(void *addr) {
    return (void *)((uint64_t)addr & ~((uint64_t)VER_NUM_BITMASK << (48 - VER_NUM_BITS)));
}
