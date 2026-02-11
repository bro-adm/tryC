#include "tryC/shift_k.h"

#define A 'A'
#define a 'a'
#define Z 'Z'
#define z 'z'
#define alpahbetsize 26

static int _modulo(int k){
    if (k >= 0) return k % alpahbetsize; // actual postive moves in a single cycle
    return ( (k % alpahbetsize) + alpahbetsize ) % alpahbetsize; // -1...-25,0 + alphabetsize => 25...1,26 => extra module alpahbetsize => 25...1,0 -> good :)
}

void shift_k(char *str, int k){
    if (!str || str[0] == '\0') return; // null pointer, empty string, no shift

    k = _modulo(k);
    if (k == 0) return;

    int i = 0, currI = 0;
    char alphabetStart = 0, alphabetEnd = 0;
    int realK = 0;

    do {
        currI = i++;
        
        if (A <= str[currI] && str[currI] <= Z ) alphabetStart = A;
        else if (a <= str[currI] && str[currI] <= z) alphabetStart = a;
        else continue;

        alphabetEnd = alphabetStart + alpahbetsize - 1;

        if (alphabetEnd - str[currI] >= k) realK = k;
        else { realK = k - (alphabetEnd - str[currI]) - 1; str[currI] = alphabetStart; };

        str[currI] += realK;
    } 
    while (str[i] != '\0'); // i = 0 pre validated on init if
}

