#include "sampler.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct sampler_t {
    double *data;
    size_t capacity, n, position;
    bool cyclic, ownData;
};

sampler_t*
sampler_fromFile( FILE* file  ) {
    static const int realloc_incr_step =1024;
    int realloc_incr =1024;

    sampler_t* sampler =malloc( sizeof(sampler_t) );
    assert( sampler );
    memset( sampler, 0, sizeof(sampler_t) );

    double *data =malloc( realloc_incr * sizeof(double) );
    assert( data );
    size_t offset =0, cap =realloc_incr;

    while( !feof( file ) ) {
        if( offset >= cap ) {
            cap += realloc_incr; realloc_incr += realloc_incr_step;
            data =realloc( data, cap * sizeof(double) );
            assert( data );
        }

        int r;
        r = fscanf( file, "%lf", (data+offset) );
        if( r == 0 ) {
            if( offset == 0 ) {
                // Skip the leading lines/header automatically
                if( fscanf( file, "%*s" ) == EOF && errno != 0 ) {
                    perror( "sample_fromFile()" );
                    goto ERR;
                }
            }
            else {
                fprintf( stderr, "sampler_fromFile(): encountered invalid value\n" );
                goto ERR;
            }
        } else if( errno != 0 ) {
            perror( "sample_fromFile()" );
            goto ERR;

        } else if( r == 1 )
            offset++;
    }

    if( offset > 0 ) {
        sampler->data     =data;
        sampler->n        =offset;
        sampler->capacity =cap;
        sampler->ownData  =true;
    } else {
        fprintf( stderr, "sampler_fromFile(): array is empty\n" );
        goto ERR;
    }
    return sampler;
ERR:
    free( data );
    return NULL;
}

void
sampler_destroy( sampler_t* sampler ) {
    assert( sampler );
    if( sampler->ownData ) free( sampler->data );
    free( sampler );
}

void
sampler_setCyclic( sampler_t* sampler, bool b ) {
    assert( sampler );
    sampler->cyclic =b;
}

// Temporary
const double*
sampler_getDataPtr( sampler_t* sampler ) {
    assert( sampler );
    return sampler->data;
}

double
sampler_getNext( sampler_t* sampler ) {
    assert( sampler );
    if( sampler->position >= sampler->n ) { 
        if( sampler->cyclic )
            sampler->position =0;
        else
            return 0.;
    }
    return sampler->data[sampler->position++];
}

void
sampler_rewind( sampler_t* sampler ) {
    assert( sampler );
    sampler->position =0;
}

bool
sampler_atEnd( sampler_t* sampler ) {
    assert( sampler );
    return sampler->position < sampler->n;
}

bool
sampler_isCyclic( sampler_t* sampler ) {
    assert( sampler );
    return sampler->cyclic;
}

size_t
sampler_length( sampler_t* sampler ) {
    assert( sampler );
    return sampler->n;
}

size_t
sampler_remaining( sampler_t* sampler ) {
    assert( sampler );
    return sampler->n - sampler->position;
}
