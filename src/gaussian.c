#include "gaussian.h" 

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

//#define SQRT2PI 2.5066282746310002

struct gaussian {
    double *kernel;
    double sigma, cutoff;
    size_t size, center;
};

gaussian_t*
gaussian_create( double sigma, double cutoff ) {
    assert( sigma > 0.0 && cutoff > 0.0 );

    gaussian_t* g = malloc( sizeof(gaussian_t) );
    assert( g != NULL );

    g->sigma      = sigma;
    g->cutoff     = cutoff;
    size_t w      = g->center = (size_t)sigma * cutoff + .5;
    g->size       = w * 2 + 1;

    g->kernel     = malloc( g->size * sizeof(double) );
    assert( g->kernel != NULL );

    // Compute a symmetric gaussian left and right from the centerpoint w
    double sigma2 = sigma*sigma;
//    double norm   = (sigma * SQRT2PI);
    double sum    = 1.;
    g->kernel[w]  = 1.;
    for( size_t i = 1; i <= w; i++ ) {
        double i2 = i*i;
        double d  = exp(-.5 * i2 / sigma2);

        g->kernel[w-i] = d;
        g->kernel[w+i] = d;
        sum += 2. * d;
    }
   
    // Normalize (equivalent to multiplying with sigma*sqrt(2*PI))
    for( size_t i = 0; i < g->size; i++ ) {
        g->kernel[i] /= sum;
    }

    return g;
}

void
gaussian_destroy( gaussian_t* g ) {
    if( g->kernel != NULL ) free( g->kernel );
    free( g );
}

const double*
gaussian_dataPtr( gaussian_t* g ) {
    return g->kernel;
}

size_t
gaussian_kernelSize( gaussian_t* g ) {
    return g->size;
}

size_t
gaussian_kernelCenter( gaussian_t* g ) {
    return g->center;
}

double
gaussian_sigma( gaussian_t* g ) {
    return g->sigma;
}

double
gaussian_cutoff( gaussian_t* g ) {
    return g->cutoff;
}
