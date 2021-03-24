// Question
// 1)Should write declaration of static function?
// 2)Best way for make attr (one function or 3)

#include "Integration.h"

#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char TOPOLOGY_PATH[] =
                            "/sys/devices/system/cpu/cpu%zu/topology/core_id";
//static const size_t MAX_LEN_NUM_CPU = 3;
static const double BEGIN = 1;
static const double END   = 500;
static const double DELTA = 0.0000001;

struct CoreInfo {
    size_t  coreId;
    size_t* numCpus;
    size_t  nBusyCpus;

    size_t  nCpus;
    size_t  nAllocCpus;
};

struct IntegrationInfo {
    double begin;
    double end;
    double delta;
};

struct ThreadInfo {
    struct IntegrationInfo II;
    double res;
    size_t numCpu;
};

//------------------------------------------------------------------------------

static struct CoreInfo* MakeCoreInfos(size_t* size);
static bool GetCoreId(size_t numCpu, size_t* coreId);

static struct CoreInfo*
FillCoreInfo(struct CoreInfo* CIs, size_t size, size_t coreId, size_t numCpu);

static struct CoreInfo* CreateNewCoreInfo(struct CoreInfo* CIs, size_t size);

static size_t
DeleteRedundantCoreInfos(struct CoreInfo** ptrCIs, size_t oldSize);

static struct CoreInfo*
FindCoreInfo(const struct CoreInfo* CIs, size_t size, size_t coreId);

static void FreeCoreInfos(struct CoreInfo* CIs, size_t size);
static void
DumpCoreInfos(const struct CoreInfo* CIs, size_t size) __attribute_used__;


static void* AllocateThreadInfos(size_t nThread, size_t* sizeofTI);
static void
FillThreadInfosIntegrationInfo(void* TIs, size_t sizeofTI, size_t nThread,
                               const struct IntegrationInfo* II);
static void
FillThreadInfosNumCpu(void* TIs, size_t sizeofTI, size_t nThread,
                      const struct CoreInfo* CIs, size_t nCoreIds);
static void
DumpThreadInfos(void* TIs, size_t sizeofTI, size_t nThread) __attribute_used__;


static void* Calculate(void* TI);

double Function(double x);

// ---- CoreInfo ---------------------------------------------------------------

static struct CoreInfo* MakeCoreInfos(size_t* size) {
    assert(size);

    long tmpNCpus = sysconf(_SC_NPROCESSORS_CONF);
    if (tmpNCpus <= 0) {
        perror("Bad nCpu");
        return NULL;
    }
    size_t nCpus = (size_t)tmpNCpus;

    struct CoreInfo* CIs = (struct CoreInfo*)calloc(nCpus, sizeof(*CIs));
    if (CIs == NULL) {
        perror("Bad calloc CIs");
        return NULL;
    }

    for (size_t iNumCpu = 0; iNumCpu < nCpus; iNumCpu++) {
        size_t curCoreId = 0;
        bool ret = GetCoreId(iNumCpu, &curCoreId);
        if (ret != true) {
            perror("Error GetCoreId");
            FreeCoreInfos(CIs, nCpus);
            return NULL;
        }

        struct CoreInfo* curCI = FillCoreInfo(CIs, nCpus, curCoreId, iNumCpu);
        if (curCI == NULL) {
            perror("Error FillCoreInfo");
            FreeCoreInfos(CIs, nCpus);
            return NULL;
        }
    }

    *size = DeleteRedundantCoreInfos(&CIs, nCpus);
    if (*size == 0) {
        perror("Error DeleteRedundantCoreInfos");
        return NULL;
    }

    return CIs;
}

static bool GetCoreId(size_t numCpu, size_t* coreId) {
    assert(coreId);

    //TODO mb size len
    char curTopologyPath[sizeof(TOPOLOGY_PATH)] = {};
    sprintf(curTopologyPath, TOPOLOGY_PATH, numCpu);

    FILE* topologyFile = fopen(curTopologyPath, "r");
    if (topologyFile == NULL)
        return false;

    int ret = fscanf(topologyFile, "%zu", coreId);
    if (ret != 1) {
        fclose(topologyFile);
        return false;
    }

    fclose(topologyFile);
    return true;
}

static struct CoreInfo*
FillCoreInfo(struct CoreInfo* CIs, size_t size, size_t coreId, size_t numCpu) {
    assert(CIs);

    struct CoreInfo* curCI = FindCoreInfo(CIs, size, coreId);

    if (curCI == NULL || curCI->nCpus == 0) {
        curCI = CreateNewCoreInfo(CIs, size);
        if (curCI == NULL)
            return NULL;
        curCI->coreId = coreId;
    }

    if (curCI->nCpus == curCI->nAllocCpus) {
        curCI->numCpus = (size_t*)realloc(curCI->numCpus,
                                       2 * curCI->nAllocCpus *
                                       sizeof(*(curCI->numCpus)));
        if (curCI->numCpus == NULL)
            return NULL;
        curCI->nAllocCpus = 2 * curCI->nAllocCpus;
    }

    curCI->numCpus[curCI->nCpus] = numCpu;
    curCI->nCpus++;

    return curCI;
}

static struct CoreInfo*
CreateNewCoreInfo(struct CoreInfo* CIs, size_t size) {
    assert(CIs);

    struct CoreInfo* curCI = NULL;
    for (size_t iNumCI = 0; iNumCI < size && !curCI; iNumCI++)
        if (CIs[iNumCI].nCpus == 0)
            curCI = &CIs[iNumCI];

    curCI->numCpus = (size_t*)calloc(1, sizeof(*(curCI->numCpus)));
    if (curCI->numCpus == NULL)
        return NULL;
    curCI->nAllocCpus = 1;

    return curCI;
}

static size_t
DeleteRedundantCoreInfos(struct CoreInfo** ptrCIs, size_t oldSize) {
    assert(ptrCIs);

    struct CoreInfo* CIs = *ptrCIs;
    size_t newSize = oldSize;
    for (size_t iNumCI = 0; iNumCI < oldSize; iNumCI++)
        if (CIs[iNumCI].nCpus == 0) {
            free(CIs[iNumCI].numCpus);
            newSize--;
        }
    CIs = (struct CoreInfo*)realloc(CIs, newSize * sizeof(*CIs));
    if (CIs == NULL)
        return 0;
    *ptrCIs = CIs;

    return newSize;
}

static struct CoreInfo*
FindCoreInfo(const struct CoreInfo* CIs, size_t size, size_t coreId) {
    assert(CIs);

    const struct CoreInfo* curCI = NULL;
    for (size_t iNumCI = 0; iNumCI < size && !curCI; iNumCI++)
        if (CIs[iNumCI].coreId == coreId)
            curCI = &CIs[iNumCI];

    return (struct CoreInfo*)curCI;
}

static void FreeCoreInfos(struct CoreInfo* CIs, size_t size) {
    assert(CIs);

    for (size_t iNumCI = 0; iNumCI < size; iNumCI++)
        free(CIs[iNumCI].numCpus);

    free(CIs);
}

static void DumpCoreInfos(const struct CoreInfo* CIs, size_t size) {
    assert(CIs);

    fprintf(stderr, "\n====== DUMP CI ======\n");
    for (size_t iNumCI = 0; iNumCI < size; iNumCI++) {
        fprintf(stderr, "\n--- Core id = %zu ---\n", CIs[iNumCI].coreId);
        fprintf(stderr, "nCpus = %zu, ",      CIs[iNumCI].nCpus);
        fprintf(stderr, "nAllocCpus = %zu, ", CIs[iNumCI].nAllocCpus);
        fprintf(stderr, "nBusyCpus = %zu:\n", CIs[iNumCI].nBusyCpus);
        fprintf(stderr, "numCpus : [");
        assert(CIs[iNumCI].numCpus);
        for (size_t iNumCpu = 0; iNumCpu < CIs[iNumCI].nCpus; iNumCpu++)
            fprintf(stderr, " %zu", CIs[iNumCI].numCpus[iNumCpu]);
        fprintf(stderr, "]\n");
    }
}

// ---- Thread Info ------------------------------------------------------------

static void* AllocateThreadInfos(size_t nThread, size_t* sizeofTI) {
    assert(sizeofTI);

    long sizeLine = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (sizeLine <= 0)
        return NULL;

    *sizeofTI = (sizeof(struct ThreadInfo) / sizeLine + 1) * sizeLine;

    void* TIs = calloc(nThread, *sizeofTI);
    return TIs;
}

static void
FillThreadInfosIntegrationInfo(void* TIs, size_t sizeofTI, size_t nThread,
                               const struct IntegrationInfo* II) {
    assert(TIs);
    assert(II);
    assert(II->end - II->begin > 0);

    double step = (II->end - II->begin) / nThread;
    for (size_t iNumThread = 0; iNumThread < nThread; iNumThread++) {
        double iBegin = II->begin + iNumThread * step;
        double iEnd   = II->begin + (iNumThread + 1) * step;
        double iDelta = II->delta;

        struct ThreadInfo* iTI = TIs + iNumThread * sizeofTI;
        iTI->II.begin = iBegin;
        iTI->II.end   = iEnd;
        iTI->II.delta = iDelta;
    }
}

static void
FillThreadInfosNumCpu(void* TIs, size_t sizeofTI, size_t nThread,
                      const struct CoreInfo* CIs, size_t nCoreIds) {
    assert(TIs);
    assert(CIs);

    for (size_t iNumThread = 0; iNumThread < nThread; iNumThread++) {
        size_t coreId = iNumThread % nCoreIds;
        struct CoreInfo* curCI = FindCoreInfo(CIs, nCoreIds, coreId);
        assert(curCI);

        size_t iNumCpu = curCI->nBusyCpus % curCI->nCpus;
        size_t curNumCpu = curCI->numCpus[iNumCpu];
        ((struct ThreadInfo*)(TIs + iNumThread * sizeofTI))->numCpu = curNumCpu;
        curCI->nBusyCpus++;
    }

}

static void DumpThreadInfos(void* TIs, size_t sizeofTI, size_t nThread) {
    assert(TIs);

    fprintf(stderr, "\n==== Dump TI ====\n");
    for (size_t iNumThread = 0; iNumThread < nThread; iNumThread++) {
        struct ThreadInfo* iTI = TIs + iNumThread * sizeofTI;
        fprintf(stderr, "---- numThread = %zu:", iNumThread);
        fprintf(stderr, "numCpu %zu\n",          iTI->numCpu);
        fprintf(stderr, "from [%f] to [%f] with delta = %f\n",
                iTI->II.begin, iTI->II.end, iTI->II.delta);
        fprintf(stderr, "res = %f\n", iTI->res);
    }
}

// ---- Other ------------------------------------------------------------------

static void* Calculate(void* ThreadInfo) {
    assert(ThreadInfo);

    struct ThreadInfo* TI = ThreadInfo;

    cpu_set_t cpu = {};
    pthread_t pthread = pthread_self();
    size_t numCpu = TI->numCpu;

    CPU_ZERO(&cpu);
    CPU_SET(numCpu, &cpu);

    int ret = pthread_setaffinity_np(pthread, sizeof(cpu), &cpu);
    if (ret != 0) {
        perror("Error pthread setaffinity");
        return NULL;
    }

    double begin  = TI->II.begin;
    double end    = TI->II.end;
    double delta  = TI->II.delta;

    double res = 0;
    for (double x = begin; x < end; x += delta)
        res += Function(x) * delta;

    TI->res = res;
    return NULL;
}

// ---- Func Database ----------------------------------------------------------

double Function(double x) {
    return x * x;
}

// ==== API ====================================================================

int Integrate(size_t nThreads, double* res) {

    if (res == NULL)
        return INTEGRATION_EINVAL;

    size_t nCoreIds = 0;
    struct CoreInfo* CIs = MakeCoreInfos(&nCoreIds);
    if (CIs == NULL)
        return INTEGRATION_ENOMEM;

    size_t sizeofTI = 0;
    void* TIs = AllocateThreadInfos(nThreads, &sizeofTI);
    if (TIs == NULL)
        return INTEGRATION_ENOMEM;

    struct IntegrationInfo II = {.begin = BEGIN,
                                 .end = END,
                                 .delta = DELTA};
    FillThreadInfosIntegrationInfo(TIs, sizeofTI, nThreads, &II);
    FillThreadInfosNumCpu         (TIs, sizeofTI, nThreads, CIs, nCoreIds);

    pthread_t* pthreads = (pthread_t*)calloc(nThreads, sizeof(*pthreads));
    if (pthreads == NULL) {
        perror("Bad calloc");
        return INTEGRATION_ENOMEM;
    }

    for (size_t iNumThread = 0; iNumThread < nThreads; iNumThread++) {
        int retInt = pthread_create(&pthreads[iNumThread], NULL, Calculate,
                                    TIs + iNumThread * sizeofTI);
        if (retInt != 0) {
            perror("Error pthread_create");
            return INTEGRATION_ESYS;
        }
    }

    for (size_t iNumThread = 0; iNumThread < nThreads; iNumThread++) {
        int ret = pthread_join(pthreads[iNumThread], NULL);
        if (ret != 0) {
            perror("Pthread join");
            return INTEGRATION_ESYS;
        }
        *res += ((struct ThreadInfo*)(TIs + iNumThread * sizeofTI))->res;
    }
    //DumpThreadInfos(TIs, sizeofTI, nThreads);

    free(pthreads);
    free(TIs);
    FreeCoreInfos(CIs, nCoreIds);

    return 0;
}
