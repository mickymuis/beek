#ifndef GAUSSIAN_H
#define GAUSSIAN_H

#include <stddef.h>

typedef struct gaussian gaussian_t;

gaussian_t*
gaussian_create( double sigma, double cutoff );

void
gaussian_destroy( gaussian_t* );

const double*
gaussian_dataPtr( gaussian_t* );

size_t
gaussian_kernelSize( gaussian_t* );

size_t
gaussian_kernelCenter( gaussian_t* );

double
gaussian_sigma( gaussian_t* );

double
gaussian_cutoff( gaussian_t* );

#endif
