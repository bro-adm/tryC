#include <stdlib.h>
#include <stdio.h>

#include "tryC/shift_k.h"

int main(int argc, char *argv[])
{
    char str[] = "a!bc"; // instead of char* str to string literal "whatever" becuase then it would a pointer to the immutable string instead of teh copied mutable string on the stack
    shift_k(str, -3);
    printf("%s\n", str);

    printf("%c, %d\n", 'A', 'A');
    return EXIT_SUCCESS;
}

