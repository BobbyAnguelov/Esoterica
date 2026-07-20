#include <cstdio>
#include <cmath>
#include <cstdlib>

int main(int argc, char **argv)
{
    if (argc > 1) {
        float val = (float)atof(argv[1]);
        printf("sin(%.2f) = %.2f\n", val, std::sin(val));
    }
    return 0;
}
