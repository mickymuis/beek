#ifndef STAGES_SOURCE_H
#define STAGES_SOURCE_H

#include "../flow.h"
#include "../sampler.h"

flw_stage_t*
flw_createSampleSource( sampler_t*, int port_size );

#endif

