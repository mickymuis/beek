#include "scope.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#define SDL_RECT_EQ(a,b) (a.x==b.x && a.y==b.y && a.w==b.w && a.h==b.h)

typedef struct {
    int mode, style;
    sampler_t* sampler;
    cfifo_t* sampleQueue;
    cfifo_t* sampleBuffer;
    float hshift;
    size_t offset;
    size_t length;
    double min, max;
    SDL_Rect area;
} schannel_t;

struct scope_t {
    schannel_t* channel;
    unsigned int nchannels;
    unsigned int speed; // samples/s
    SDL_Rect area;
    SDL_Texture* backdrop;
    uint64_t time;
    double elapsed, ticksPerSec;
};

scope_t*
scope_create( unsigned int nchannels ) {
    scope_t* s   = malloc( sizeof(scope_t) );
    assert( s );
    memset( s, 0, sizeof( scope_t ) );
    s->nchannels = nchannels;
    s->channel   = malloc( nchannels * sizeof(schannel_t) );
    assert( s->channel );
    memset( s->channel, 0, nchannels * sizeof(schannel_t) );
    for( int i =0; i < nchannels; i++ )
        scope_initChannel( s, i, SCOPE_CHANNEL_NONE );

    s->ticksPerSec = (double)SDL_GetPerformanceFrequency();
    return s;
}

void
scope_destroy( scope_t* s ) {
    if( s == NULL ) return;
    if( s->backdrop != NULL )
        SDL_DestroyTexture( s->backdrop );
    if( s->channel  != NULL ) {
        for( int i =0; i < s->nchannels; i++ ) {
            if( s->channel[i].sampleQueue != NULL )
                cfifo_destroy( s->channel[i].sampleQueue );
            if( s->channel[i].sampleBuffer != NULL )
                cfifo_destroy( s->channel[i].sampleBuffer );
        }
        free( s->channel );
    }
    free( s );
}

void
scope_setSpeed( scope_t* scope, unsigned int speed ) {
    assert( scope );
    scope->speed = speed;
}

void
scope_initChannel( scope_t* scope, unsigned int chan, int mode ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];
    c->hshift =.0f;
    c->offset =0;
    c->length =10000;
    c->max    =1.0;
    c->min    =-1.0;
    c->mode   =mode;

    if( c->sampleQueue  != NULL ) cfifo_destroy( c->sampleQueue );
    if( c->sampleBuffer != NULL ) cfifo_destroy( c->sampleBuffer );

    switch( mode ) {
        case SCOPE_CHANNEL_STATIC:
            c->sampleBuffer =cfifo_create( SCOPE_MAX_LENGTH );
            break;
        case SCOPE_CHANNEL_STREAM:
            c->sampleBuffer =cfifo_create( SCOPE_MAX_LENGTH );
            c->sampleQueue  =cfifo_create( SCOPE_FIFO_DEPTH );
            break;
        default:
            break;
    }
}

void
scope_setChannelBuffer( scope_t* scope, unsigned int chan, sampler_t* sampler ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];
    if( c->mode != SCOPE_CHANNEL_STATIC ) return;

    c->sampler =sampler;
}

void
scope_pushChannelFifo( scope_t* scope, unsigned int chan, double sample ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];
    if( c->mode != SCOPE_CHANNEL_STREAM ) return;

    cfifo_push( c->sampleQueue, sample );
}

void
scope_updateChannel( scope_t* scope, unsigned int chan ) {
    schannel_t* c =&scope->channel[chan];
    if( c->mode == SCOPE_CHANNEL_NONE ) return;

    // Compute the amount of samples that the view has shifted since the last draw
    c->hshift += (float)scope->speed * scope->elapsed;
    // Whole number of new samples 
    int delta = (int)c->hshift;
    // Decimal part
    c->hshift -= delta;
    // Shift the offset into the sample array accordingly (wrap around if necessary)
    //c->offset = (c->offset + delta) % sampler_length( c->sampler );

    // Less than one new samples to consume, return
    if( delta == 0 ) return;
        
    for( int i=0; i < delta; i++ ) {
        double s =0.;
        // Consume the sample from the appropriate buffer/fifo
        if( c->mode == SCOPE_CHANNEL_STATIC ) {
            s = sampler_getNext( c->sampler );
        } else if( c->mode == SCOPE_CHANNEL_STREAM ) {
            s = cfifo_take( c->sampleQueue );
        }

        // samplerBuffer will be used to draw the plot
        cfifo_push( c->sampleBuffer, s );
        if( cfifo_length( c->sampleBuffer ) > c->length )
            cfifo_take( c->sampleBuffer );
    }
}

void
scope_setChannelDrawStyle( scope_t* scope, unsigned int chan, int style ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];

    c->style =style;
}

void
scope_drawGrid( SDL_Renderer* render, SDL_Rect r, int vdiv ) {
    int step    =r.h / (vdiv+1);                        // Size of one 'box' in the grid
    int y       =r.y + step;                            // Y position of the first gridline
    int hdiv    =r.w / step;                            // Number of horizontal divisions
    int xbegin  =r.x + step + (r.w % step) / 2;         // X position of the first gridline
    int len     =r.h / 80;                              // Half length of a tickmark

    for( int ydiv =0; ydiv < vdiv; ydiv++ ) {
        int x   =xbegin;
    //    SDL_RenderDrawLine( render, r.x, y, r.x + r.w, y );
        for( int xdiv =0; xdiv < hdiv-1; xdiv++ ) {
    //        SDL_RenderDrawLine( render, x, r.y, x, r.y+r.h );
            SDL_RenderDrawLine( render, x - len, y, x + len, y );
            SDL_RenderDrawLine( render, x , y - len , x, y + len );
            
            SDL_RenderDrawLine( render, x - len + step/2, y, x + len + step/2, y );
            SDL_RenderDrawLine( render, x, y - len + step/2, x, y + len + step/2 );

            if( ydiv == 0 )
                SDL_RenderDrawLine( render, x, y - len - step/2, x, y + len - step/2 );

            if( xdiv == 0 )
                SDL_RenderDrawLine( render, x - len - step/2, y, x + len - step/2, y);

            x  +=step;
        }
        y      +=step;
    }
}

void
scope_drawChannel( scope_t* scope, unsigned int chan, SDL_Renderer* render ) {
    schannel_t* c =&scope->channel[chan];
    if( c->mode == SCOPE_CHANNEL_NONE ) return;
    SDL_Rect r    =c->area;

    double min =-.1, max =.1;
    float xStep =(float)r.w / (float)c->length;
    float yStep =(float)r.h / (c->max - c->min);
    float yMid =(float)r.y + yStep * c->max;

    SDL_Point points[c->length];        // buffer the calls to SDL_RenderDrawPoint/Line
    size_t p =0;                        // n-th point

    double damp =fmin( 1.0, 100. / fmin( c->length, cfifo_length( c->sampleBuffer ) ) ); // Dampening factor for auto-scaling

    for( size_t i =0; i < c->length; i++ ) {
        // Wrap the offset around if necessary
        //size_t idx = (c->offset + i) % sampler_length( c->sampler );

        // Sample value from the sample buffer
        //double val =sampler_getDataPtr( c->sampler )[idx];

        double val =cfifo_read( c->sampleBuffer, i );

        // Coordinates within the given clip rectangle
        float x =((float)i - c->hshift)*xStep + (float)r.x;
        float y =yMid - fmax( c->min, fmin( c->max, val ) ) * yStep;

        // Draw...
        if( (int)x >= r.x) {
            //SDL_RenderDrawPoint( render, x, y );
            SDL_Point pnt = {x,y};
            points[p++] = pnt;
        }

        // Adjust min and max, but try to avoid one single spike throwing the image off
        if( val > 0 && val > max ) {
            double f = fmin(1.0, (max / val)) * damp;
            max =max * (1.-f) + val * f;
        }
        else if( val < 0 && val < min ) {
            double f = fmax(1.0, -(min / val)) * damp;
            min =min * (1.-f) + val * f;
        }
    }

    if( c->style == SCOPE_CHANNEL_LINES )
        SDL_RenderDrawLines( render, points, p );
    SDL_RenderDrawPoints( render, points, p );

    // Auto-zoom with damping/interpolation, factor in speed
    damp *=scope->speed * 0.001;
    c->max =c->max * (1.-damp) + max * damp;
    c->min =c->min * (1.-damp) + min * damp;
}

void
scope_computeElapsedTime( scope_t* scope ) {
    uint64_t now = SDL_GetPerformanceCounter();
    if( scope->time == 0UL ) {
        scope->elapsed =0.f;
    } else {
        scope->elapsed =(double)(now - scope->time) / scope->ticksPerSec;
    }

    scope->time =SDL_GetPerformanceCounter();
}

void
scope_update( scope_t* scope, SDL_Renderer* render, SDL_Rect area ) {
    // Compute the time between this and the last call 
    scope_computeElapsedTime( scope );
    
    // First we update every channel based on the elapsed time between calls to scope_update()
    for( int i =0; i < scope->nchannels; i++ )
        scope_updateChannel( scope, i );
    
    // Window size or draw area has changed, we need to update the backdrop texture
    if( !SDL_RECT_EQ( area, scope->area ) || scope->backdrop == NULL ) {

        // Render the backdrop to a texture
        if( scope->backdrop != NULL )
            SDL_DestroyTexture( scope->backdrop );

        scope->backdrop = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, area.w, area.h);
        SDL_SetRenderTarget( render, scope->backdrop );
        SDL_SetRenderDrawColor( render, 0x00, 0x00, 0x00, 0x00 );
        SDL_RenderClear( render );
        
        // First, calculate the positions of the channels
        int pad =10;

        int height = (area.h - pad) / scope->nchannels - pad;
        int width = area.w - 2*pad;
        int x =pad + area.x, y =pad + area.y;
        for( int i =0; i < scope->nchannels; i++ ) {
            //if( scope->channel[i].mode == SCOPE_CHANNEL_NONE ) continue; 
            // Absolute position/size within the render area
            SDL_Rect r = {x, y, width, height};
            scope->channel[i].area =r;

            // Render the frame and the grid for this channel to the texture
            SDL_SetRenderDrawColor( render, 15, 96, 63, SDL_ALPHA_OPAQUE );
            scope_drawGrid( render, r, 8 );
            SDL_RenderDrawRect( render, (SDL_Rect*)&r );
        //    if( (int)yMid < r.y+r.h-10 && (int)yMid > r.y+10 )
          //      SDL_RenderDrawLine( render, r.x, yMid, r.x+r.w, yMid );

            y += height + pad;
        }
        scope->area =area;
        
        SDL_SetRenderTarget( render, NULL );
    }

    // Draw the backdrop texture
    SDL_RenderCopy( render, scope->backdrop, NULL, &area );

    // Draw the channels
    SDL_SetRenderDrawColor( render, 15, 255, 63, SDL_ALPHA_OPAQUE );
    for( int i =0; i < scope->nchannels; i++ )
        scope_drawChannel( scope, i, render );

}
