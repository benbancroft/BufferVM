#include "../libc/stdlib.h"

static int numbers[] = {5, 7, 11, 13, 6};

const int ver_num_bits = 4;

void *set_version(uint64_t version, uint64_t *pointer) {
    return (void *)((uint64_t) pointer | ((version & 0xF) << (48 - ver_num_bits)));
}

uint64_t get_version(uint64_t *pointer) {
    return (uint64_t)pointer >> (48 - ver_num_bits);
}

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
    printf("String: %s, Ver: %d\n", my_string, (int)get_version(my_string));
    ver_string = set_version(5, my_string);
    printf("Versioned string: %s Ver: %d\n", ver_string, (int)get_version(ver_string));

    //read test cases

    printf("\nPlease enter a character: \n");

    //char *name = getline();
    //printf("My name is: %s", name);

    printf("Your character: %c\n", fgetc(1));

    return;
}
