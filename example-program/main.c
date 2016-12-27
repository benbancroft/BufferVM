#include "../libc/stdlib.h"
#include "alloc.h"
#include "../common/version.h"

static int numbers[] = {5, 7, 11, 13, 6};

int sum_array(int a[], int num_elements) {
    int sum = 0;
    for (int i = 0; i < num_elements; i++) {
        sum = sum + a[i];
    }
    return sum;
}

void main() {
    char *my_string, *ver_string;

    //printf/std test cases

    int ret = sum_array(numbers, sizeof(numbers) / sizeof(int));
    printf("Hello world from C! - Number is: %d\n", ret);

    write(1, "hi write\n", 9);

    printf("Two ");
    printf("Lines");
    printf("\n");

    putchar('b');
    putchar('\n');

    //0xDEADBEAF
    printf("Pointer: 0x%x!\n", 3735928495);

    //version test case

    my_string = "epic string";
    printf("String: %s, Ver: %d\n", my_string, (int)get_version_ptr(my_string));
    ver_string = set_version_ptr(5, my_string);
    printf("Versioned string: %s Ver: %d\n", ver_string, (int)get_version_ptr(ver_string));

    //heap test

    char *mem = malloc(10);
    printf("Heap: %p\n", mem);

    mem = set_version(mem, 10, 5);
    printf("Versioned mem - Ver: %d prt: %p\n", (int)get_version_ptr(mem), mem);
    mem[9] = 42;

    //file stuff
    int32_t file_handle = open("test.txt", 0, 0);
    printf("open file %d\n", file_handle);
    char file_contents[50];
    read(file_handle, file_contents, 50);
    printf("File: %s\n", file_contents);
    printf("close file %d\n", close(file_handle));

    //read test case

    /*printf("\nPlease enter a character: \n");
    printf("Your character: %c\n", fgetc(1));*/

    printf("\nPlease enter your name: \n");
    char *name = getline();
    printf("My name is: %s", name);

    printf("yey\n");
    return;
}
