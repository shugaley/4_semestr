
#include "Integration.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char TOPOLOGY_PATH[] =
                            "/sys/devices/system/cpu/cpu%zu/topology/core_id";

struct CoreInfo {
    size_t  coreId;
    size_t* cpus;
    size_t  nBusyCpus;

    size_t  nCpus;
    size_t  nAllocCpus;
};

struct ThreadInfo {
    double begin;
    double end;
    double delta;
    double res;
};

//------------------------------------------------------------------------------

static struct CoreInfo* MakeCoreInfos(size_t* size);
static struct CoreInfo*
FillCoreInfo(struct CoreInfo* CIs, size_t size, size_t coreId, size_t numCpu);
static struct CoreInfo* CreateNewCoreInfo(struct CoreInfo* CIs, size_t size);
static size_t
DeleteRedundantCoreInfos(struct CoreInfo** ptrCIs, size_t oldSize);
static void FreeCoreInfos(struct CoreInfo* CIs, size_t size);
static void
DumpCoreInfo(const struct CoreInfo* CIs, size_t size) __attribute_used__;

static void* AllocateThreadInfos(size_t nThread, size_t* sizeOf);


// ---- CoreInfo ---------------------------------------------------------------

static struct CoreInfo* MakeCoreInfos(size_t* size) {
    assert(size);

    long nCpus = sysconf(_SC_NPROCESSORS_CONF);
    if (nCpus <= 0)
        return 0;

    struct CoreInfo* CIs = (struct CoreInfo*)calloc((size_t)nCpus,
                                                    sizeof(*CIs));
    if (CIs == NULL)
        return 0;

    for (size_t iNumCpu = 0; iNumCpu < (size_t)nCpus; iNumCpu++) {
        char curTopologyPath[sizeof(TOPOLOGY_PATH) + 10] = {};
        sprintf(curTopologyPath, TOPOLOGY_PATH, iNumCpu);
        FILE* topologyFile = fopen(curTopologyPath, "r");
        size_t curCoreId = 0;
        fscanf(topologyFile, "%zu", &curCoreId);
        fclose(topologyFile);

        struct CoreInfo* curCI = FillCoreInfo(CIs, (size_t)nCpus,
                                              curCoreId, iNumCpu);
        if (curCI == NULL) {
            FreeCoreInfos(CIs, (size_t)nCpus);
            return NULL;
        }
    }

    *size = DeleteRedundantCoreInfos(&CIs, (size_t)nCpus);
    if (*size == 0)
        return NULL;

    return CIs;
}

static struct CoreInfo*
FillCoreInfo(struct CoreInfo* CIs, size_t size, size_t coreId, size_t numCpu) {
    assert(CIs);

    struct CoreInfo* curCI = NULL;
    for (size_t iNumCI = 0; iNumCI < size && !curCI; iNumCI++)
        if (CIs[iNumCI].coreId == coreId)
            curCI = &CIs[iNumCI];

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

// ---- Other ------------------------------------------------------------------

static void* AllocateThreadInfos(size_t nThread, size_t* sizeOf) {
    assert(sizeOf);

    long sizeLine = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (sizeLine <= 0)
        return NULL;

    *sizeOf = sizeof(struct ThreadInfo) / sizeLine + 1;

    void* TIs = calloc(nThread, *sizeOf);
    return TIs;
}

// ==== API ====================================================================

int Integrate(size_t nThreads, size_t* res) {

    if (res == NULL)
        return INTEGRATION_EINVAL;

    size_t size = 0;
    struct CoreInfo* CIs = MakeCoreInfos(&size);
    if (CIs == NULL)
        return INTEGRATION_ENOMEM;

    size_t sizeofTI = 0;
    void* TIs = AllocateThreadInfos(nThreads, &sizeofTI);
    if (TIs == NULL)
        return INTEGRATION_ENOMEM;

    free(TIs);
    FreeCoreInfos(CIs, size);

    return 0;
}