#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc > 1) {
        float val = (float)atof(argv[1]);
        printf("sin(%.2f) = %.2f\n", val, sinf(val));
    }
    return 0;
}
