#ifndef SAMPLER_H
#define SAMPLER_H

#include <stdio.h>
#include <stdbool.h>

typedef struct sampler_t sampler_t;

sampler_t*
sampler_fromFile( FILE* file  );

void
sampler_destroy( sampler_t* );

void
sampler_setCyclic( sampler_t*, bool );

// Temporary
const double*
sampler_getDataPtr( sampler_t* );

double
sampler_getNext( sampler_t* );

void
sampler_rewind( sampler_t* );

bool
sampler_atEnd( sampler_t* );

bool
sampler_isCyclic( sampler_t* );

size_t
sampler_length( sampler_t* );

size_t
sampler_remaining( sampler_t* );


#endif
