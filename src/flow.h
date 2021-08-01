#ifndef FLOW_H
#define FLOW_H

#include "atomq.h"
#include "stage.h"
#include <stdbool.h>

#define FLW_DEFAULT_FIFO_DEPTH 256

typedef struct flw flw_t;

flw_t*
flw_create();

void
flw_destroy( flw_t* flw );

void
flw_addStage( flw_t* flw, flw_stage_t* stage );

void
flw_removeStage( flw_t* flw, flw_stage_t* stage );

bool
flw_connect( flw_t* flw, flw_stage_t* stage1, flw_stage_t* stage2 );

void
flw_disconnect( flw_t* flw, flw_stage_t* stage1, flw_stage_t* stage2 );

void
flw_setFrequency( flw_t* flw, int freq );

void
flw_start( flw_t* flw );

void
flw_stop( flw_t* flw );

void
flw_printGraph( flw_t* flw );

#endif
