#include "stdlib.h"

static int numbers[] = {5, 7, 11, 13, 6};

int sum_array(int a[], int num_elements) {
    int sum = 0;
    for (int i = 0; i < num_elements; i++) {
        sum = sum + a[i];
    }
    return sum;
}

void main() {
    int ret = sum_array(numbers, sizeof(numbers) / sizeof(int));

    printf("Hello world from C! - Number is: %d\n", ret);

    printf("%d %d %d\n", 3, 2, 1);

    printf("We are now in business!\n");

    printf("Two ");
    printf("Lines");
    printf("\n");

    //0xDEADBEAF
    printf("Pointer: 0x%x!\n", 3735928495);

    char *name = getline();

    printf("My name is: %s", name);

    putchar('b');
    putchar('\n');

    write(1, "hi write\n", 9);

    printf("boo!\n");

    return;
}
