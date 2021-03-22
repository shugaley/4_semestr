
#include "Integration.h"

#include "errno.h"
#include "stdio.h"
#include "stdlib.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        perror("Bad args");
        exit(EXIT_FAILURE);
    }

    char* endptr = NULL;
    size_t nThreads = strtoul(argv[1], &endptr, 10);

    if (errno != 0 || *endptr != '\0' || nThreads <= 0) {
        perror("Bad args");
        exit(EXIT_FAILURE);
    }

    size_t res = 0;
    int ret = Integrate(nThreads, &res);
    if (ret != 0)
        exit(EXIT_FAILURE);
    printf("%zu\n", res);

    return 0;
}