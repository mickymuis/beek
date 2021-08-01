#ifndef STAGES_SINK_H
#define STAGES_SINK_H

#include "../flow.h"
#include "../scope.h"

flw_stage_t*
flw_createScopeSink( scope_t*, unsigned int channel, int port_size );

#endif
