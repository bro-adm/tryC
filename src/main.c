#include <stdio.h>
#include <stdlib.h>

#include "tryC/bitops.h"

int main(int argc, char *argv[])
{
    int a=8, b=9;

    printf("--- BEFORE SWAP ---\n");
    printf("Address of a: %p  |  Value of a: %d\n", (void*)&a, a);
    printf("Address of b: %p  |  Value of b: %d\n", (void*)&b, b);

    swap(&a, &b, sizeof(int));

    printf("--- AFTER SWAP ---\n");
    printf("Address of a: %p  |  Value of a: %d\n", (void*)&a, a);
    printf("Address of b: %p  |  Value of b: %d\n", (void*)&b, b);

    return EXIT_SUCCESS;
}

