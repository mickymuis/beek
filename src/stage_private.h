#ifndef STAGE_PRIVATE_H
#define STAGE_PRIVATE_H

#include "flow.h"
#include <stdatomic.h>

typedef void(*flw_consume_func_t)(void*);
typedef void(*flw_destroy_func_t)(void*);

struct flw_stage {
    const char*         name;
    int                 inPortSize;
    int                 outPortSize;
    atomq_t*            inq;
    atomq_t*            outq;
    flw_consume_func_t  consume;
    flw_destroy_func_t  destroy;
    bool                runAsThread, synchronize;
    bool                running;
    atomic_bool         stop; 
    atomic_flag         syncIdle;
    flw_stage_t*        inStage;
    flw_stage_t*        outStage;
};

/** Sets the memory occupied by a stage_t to zero and initializes its atomic variables.
 */
void
flw_initStage( flw_stage_t* );

#endif
