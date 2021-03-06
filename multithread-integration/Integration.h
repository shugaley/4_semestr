#ifndef MULTITHREAD_INTEGRATION_INTEGRATION_H
#define MULTITHREAD_INTEGRATION_INTEGRATION_H

#include <stddef.h>

enum IntegrationErrno {
    INTEGRATION_EINVAL = 1,
    INTEGRATION_ENOMEM = 2,
    INTEGRATION_ESYS   = 3,
};


int Integrate(size_t nThreads, double* res);

#endif // MULTITHREAD_INTEGRATION_INTEGRATION_H