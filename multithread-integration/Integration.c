// Question
// 1)Should write declaration of static function?
// 2)Best way for make attr (one function or 3)

#include "Integration.h"

#define _GNU_SOURCE_
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char TOPOLOGY_PATH[] =
                            "/sys/devices/system/cpu/cpu%zu/topology/core_id";
static const size_t MAX_LEN_NUM_CPU = 3;
static const double BEGIN = 1;
static const double END   = 10;
static const double DELTA = 0.00001;

struct CoreInfo {
    size_t  coreId;
    size_t* cpus;
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
};

//------------------------------------------------------------------------------

static struct CoreInfo* MakeCoreInfos(size_t* size);

static struct CoreInfo*
FillCoreInfo(struct CoreInfo* CIs, size_t size, size_t coreId, size_t numCpu);

static struct CoreInfo* CreateNewCoreInfo(struct CoreInfo* CIs, size_t size);

static size_t
DeleteRedundantCoreInfos(struct CoreInfo** ptrCIs, size_t oldSize);

static struct CoreInfo*
FindCoreInfo(struct CoreInfo* CIs, size_t size, size_t coreId);

static void FreeCoreInfos(struct CoreInfo* CIs, size_t size);

static void
DumpCoreInfo(const struct CoreInfo* CIs, size_t size) __attribute_used__;


static void* AllocateThreadInfos(size_t nThread, size_t* sizeofTI);
static void
PrepareThreadInfos(void* TIs, size_t sizeofTI, size_t nThread,
                   const struct IntegrationInfo* II);

static bool MakePthreadAttr(pthread_attr_t* attr, struct CoreInfo* CI);

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
        char curTopologyPath[sizeof(TOPOLOGY_PATH) + MAX_LEN_NUM_CPU] = {};
        sprintf(curTopologyPath, TOPOLOGY_PATH, iNumCpu);

        FILE* topologyFile = fopen(curTopologyPath, "r");
        if (topologyFile == NULL) {
            perror("Error open topologyFile");
            FreeCoreInfos(CIs, nCpus);
            return NULL;
        }
        size_t curCoreId = 0;
        int ret = fscanf(topologyFile, "%zu", &curCoreId);
        if (ret != 1) {
            perror("Error fscanf topologyFile");
            fclose(topologyFile);
            FreeCoreInfos(CIs, nCpus);
            return NULL;
        }
        fclose(topologyFile);

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
        curCI->cpus = (size_t*)realloc(curCI->cpus,
                                       2 * curCI->nAllocCpus *
                                       sizeof(*(curCI->cpus)));
        if (curCI->cpus == NULL)
            return NULL;
        curCI->nAllocCpus = 2 * curCI->nAllocCpus;
    }

    curCI->cpus[curCI->nCpus] = numCpu;
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

    curCI->cpus = (size_t*)calloc(1, sizeof(*(curCI->cpus)));
    if (curCI->cpus == NULL)
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
            free(CIs[iNumCI].cpus);
            newSize--;
        }
    CIs = (struct CoreInfo*)realloc(CIs, newSize * sizeof(*CIs));
    if (CIs == NULL)
        return 0;
    *ptrCIs = CIs;

    return newSize;
}

static struct CoreInfo*
FindCoreInfo(struct CoreInfo* CIs, size_t size, size_t coreId) {
    assert(CIs);

    struct CoreInfo* curCI = NULL;
    for (size_t iNumCI = 0; iNumCI < size && !curCI; iNumCI++)
        if (CIs[iNumCI].coreId == coreId)
            curCI = &CIs[iNumCI];

    return curCI;
}

static void FreeCoreInfos(struct CoreInfo* CIs, size_t size) {
    assert(CIs);

    for (size_t iNumCI = 0; iNumCI < size; iNumCI++)
        free(CIs[iNumCI].cpus);

    free(CIs);
}

static void DumpCoreInfo(const struct CoreInfo* CIs, size_t size) {
    assert(CIs);

    fprintf(stderr, "\n====== DUMP CI ======\n");
    for (size_t iNumCI = 0; iNumCI < size; iNumCI++) {
        fprintf(stderr, "\n--- Core id = %zu ---\n", CIs[iNumCI].coreId);
        fprintf(stderr, "nCpus = %zu, ",      CIs[iNumCI].nCpus);
        fprintf(stderr, "nAllocCpus = %zu, ", CIs[iNumCI].nAllocCpus);
        fprintf(stderr, "nBusyCpus = %zu:\n", CIs[iNumCI].nBusyCpus);
        fprintf(stderr, "cpus : [");
        assert(CIs[iNumCI].cpus);
        for (size_t iNumCpu = 0; iNumCpu < CIs[iNumCI].nCpus; iNumCpu++)
            fprintf(stderr, " %zu", CIs[iNumCI].cpus[iNumCpu]);
        fprintf(stderr, "]\n");
    }
}

// ---- Thread Info ------------------------------------------------------------

static void* AllocateThreadInfos(size_t nThread, size_t* sizeofTI) {
    assert(sizeofTI);

    long sizeLine = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (sizeLine <= 0)
        return NULL;

    *sizeofTI = sizeof(struct ThreadInfo) / sizeLine + 1;

    void* TIs = calloc(nThread, *sizeofTI);
    return TIs;
}

static void
PrepareThreadInfos(void* TIs, size_t sizeofTI, size_t nThread,
                   const struct IntegrationInfo* II) {
    assert(TIs);
    assert(II);
    assert(II->end - II->begin > 0);

    double step = (II->end - II->begin) / nThread;
    for (size_t iNumThread = 0; iNumThread < nThread; iNumThread++) {
        double iBegin = II->begin + iNumThread * step;
        double iEnd   = II->begin + (iNumThread + 1) * step;
        double  iDelta = II->delta;

        ((struct ThreadInfo*)(TIs + iNumThread * sizeofTI))->II.begin = iBegin;
        ((struct ThreadInfo*)(TIs + iNumThread * sizeofTI))->II.end   = iEnd;
        ((struct ThreadInfo*)(TIs + iNumThread * sizeofTI))->II.delta = iDelta;
    }
}

// ---- Other ------------------------------------------------------------------

static bool MakePthreadAttr(pthread_attr_t* attr, struct CoreInfo* CI) {
    assert(attr);
    assert(CI);

    size_t iNumCpu = CI->nBusyCpus % CI->nCpus;
    size_t cpu = CI->cpus[iNumCpu];

    cpu_set_t cpuSet = {};
    CPU_ZERO(&cpuSet);
    CPU_SET(cpu, &cpuSet);
//TODO
}

// ---- Func Database ----------------------------------------------------------

double Function(double x) {
    return x * x;
}

// ==== API ====================================================================

int Integrate(size_t nThreads, size_t* res) {

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
    PrepareThreadInfos(TIs, sizeofTI, nThreads, &II);

    for (size_t iNumThread = 0; iNumThread < nThreads; iNumThread++) {
        size_t coreId = iNumThread % nCoreIds;
        struct CoreInfo* curCI = FindCoreInfo(CIs, nCoreIds, coreId);
        if (curCI == NULL)
            return INTEGRATION_ENOMEM;


    }

    free(TIs);
    FreeCoreInfos(CIs, nCoreIds);

    return 0;
}
