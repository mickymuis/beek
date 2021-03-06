#include "scope.h"
#include "atomq.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "SDL_FontCache/SDL_FontCache.h"

#define SDL_RECT_EQ(a,b) (a.x==b.x && a.y==b.y && a.w==b.w && a.h==b.h)

typedef struct {
    int           mode, style, vrange;
    sampler_t*    sampler;
    atomq_t*      sampleQueue;
    cfifo_t*      sampleBuffer;
    float         hshift;
    size_t        length;
    double        min, max, avg;
    SDL_Rect      area;            // actual drawing area
    uint64_t      asamples;        // used to compute actual frequency
    uint64_t      atime;           // see above
    float         afreq;           // actual frequency
    int           masterChannel;   // channel to lock vrange to
    unsigned int  samplesize;
} schannel_t;

struct scope_t {
    schannel_t*   channel;
    unsigned int  nchannels;
    unsigned int  freq;         // samples/s
    SDL_Rect      area;
    SDL_Texture*  backdrop;
    SDL_Renderer* render;
    FC_Font*      font;
    uint64_t      time;
    double        elapsed, ticksPerSec;
};

#define N_COLORS 9
static uint8_t Palette[N_COLORS][3] = {
    { 15,  96,  63  },  // dull green
    { 15,  255, 63  },  // green
    { 255, 127,  15 },  // amber
    { 196, 63,  255 },  // violet
    { 255, 235, 45  },  // yellow
    { 63,  63,  255 },  // blue
    { 255, 15,  96  },  // fushia
    { 15,  255, 215 },  // cyan
    { 255, 255, 215 }   // off-white
};

static void
setSDLPaletteColor( scope_t* scope, int index ) {
    SDL_SetRenderDrawBlendMode( scope->render, SDL_BLENDMODE_ADD );
    SDL_SetRenderDrawColor( scope->render, 
            Palette[index % N_COLORS][0], 
            Palette[index % N_COLORS][1], 
            Palette[index % N_COLORS][2], SDL_ALPHA_OPAQUE );
    /*SDL_SetRenderDrawColor( scope->render, 
            (127 + (127 * index)) % 256, 
            (255 + (85 * index)) % 256, 
            (128 + (64 * index)) % 256, 
            SDL_ALPHA_OPAQUE );*/
}

scope_t*
scope_create( unsigned int nchannels, SDL_Renderer* render ) {
    scope_t* s   = malloc( sizeof(scope_t) );
    assert( s );
    memset( s, 0, sizeof( scope_t ) );
    s->nchannels = nchannels;
    s->channel   = malloc( nchannels * sizeof(schannel_t) );
    assert( s->channel );
    memset( s->channel, 0, nchannels * sizeof(schannel_t) );
    for( int i =0; i < nchannels; i++ )
        scope_initChannel( s, i, SCOPE_MODE_NONE );

    s->ticksPerSec = (double)SDL_GetPerformanceFrequency();
    s->render      = render;
    s->font        = FC_CreateFont();

    FC_LoadFont( s->font, s->render, "src/SDL_FontCache/test/fonts/FreeSans.ttf", 12, FC_MakeColor(200,200,200,255), TTF_STYLE_NORMAL );
    
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
                atomq_destroy( s->channel[i].sampleQueue );
            if( s->channel[i].sampleBuffer != NULL )
                cfifo_destroy( s->channel[i].sampleBuffer );
        }
        free( s->channel );
    }
    if( s->font != NULL )
        FC_FreeFont( s->font );
    free( s );
}

unsigned int
scope_channelCount( scope_t* scope ) {
    assert( scope );
    return scope->nchannels;
}

void
scope_setFrequency( scope_t* scope, unsigned int freq ) {
    assert( scope );
    scope->freq = freq;
}

void
scope_initChannel( scope_t* scope, unsigned int chan, int mode ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];
    c->hshift      =.0f;
    c->length      =1000;
    c->max         =1.0;
    c->min         =-1.0;
    c->avg         =0.;
    c->mode        =mode;
    c->samplesize =1;

    if( c->sampleQueue  != NULL ) atomq_destroy( c->sampleQueue );
    if( c->sampleBuffer != NULL ) cfifo_destroy( c->sampleBuffer );

    switch( mode ) {
        case SCOPE_MODE_STATIC:
            c->sampleBuffer =cfifo_create( SCOPE_MAX_LENGTH );
            break;
        case SCOPE_MODE_STREAM:
            c->sampleBuffer =cfifo_create( SCOPE_MAX_LENGTH );
            c->sampleQueue  =atomq_create( SCOPE_FIFO_DEPTH );
            break;
        default:
            break;
    }
}

void
scope_setChannelSampleSize( scope_t* scope, unsigned int chan, unsigned int sampleSize ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    if( sampleSize < 1 ) return;
    
    schannel_t* c =&scope->channel[chan];
    c->samplesize =sampleSize;

}

unsigned int
scope_channelSampleSize( scope_t* scope, unsigned int chan ) {
    assert( scope );
    if( chan >= scope->nchannels ) return 0;
    
    schannel_t* c =&scope->channel[chan];
    return c->samplesize;
}

void
scope_setChannelBuffer( scope_t* scope, unsigned int chan, sampler_t* sampler ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];
    if( c->mode != SCOPE_MODE_STATIC ) return;

    c->sampler =sampler;
}

bool
scope_pushChannelFifo( scope_t* scope, unsigned int chan, double sample ) {
    assert( scope );
    if( chan >= scope->nchannels ) return false;
    schannel_t* c =&scope->channel[chan];
    if( c->mode != SCOPE_MODE_STREAM ) return false;

    return atomq_enqueue( c->sampleQueue, sample );
}

void
scope_setChannelDrawStyle( scope_t* scope, unsigned int chan, int style ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];

    c->style =style;
}

void
scope_setChannelLength( scope_t* scope, unsigned int chan, unsigned int length ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];

    c->length =length;
}

void
scope_setChannelVerticalRange( scope_t* scope, unsigned int chan, double min, double max ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];

    c->min =min; c->max =max;
}

void 
scope_setChannelVerticalMode( scope_t* scope, unsigned int chan, int mode ) {
    assert( scope );
    if( chan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];

    c->vrange =mode;
}

void
scope_lockChannelRange( scope_t* scope, unsigned int chan, unsigned int toChan ) {
    assert( scope );
    if( chan >= scope->nchannels || toChan >= scope->nchannels ) return;
    schannel_t* c =&scope->channel[chan];

    c->vrange =SCOPE_VRANGE_LOCKED;
    c->masterChannel =toChan;
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
scope_drawChannel( scope_t* scope, unsigned int chan ) {
    schannel_t* c =&scope->channel[chan];
    if( c->mode == SCOPE_MODE_NONE ) return;
    SDL_Rect r    =c->area;
    
//    double damp =fmin( 1.0, 1000. / fmin( c->length, cfifo_length( c->sampleBuffer ) ) ); // Dampening factor for auto-scaling
    double spread =1. / fmin( c->length+1, cfifo_length( c->sampleBuffer )+1 ); 

    double min =0., max =0.;
    if( c->vrange == SCOPE_VRANGE_LOCKED ) {
        // Obtain min, max and length from the master channel
        schannel_t* c2 =&scope->channel[c->masterChannel];
        c->min =c2->min; c->max =c2->max;
        // c->length =c2->length;
         
    } else {
        min =c->avg; max =c->avg;
    }

    float xStep =(float)r.w / (float)c->length;
    float yStep =(float)r.h / (c->max - c->min);
    float yMid =(float)r.y + yStep * c->max;

    SDL_Point points[c->samplesize][c->length]; // buffer the calls to SDL_RenderDrawPoint/Line
    size_t p[c->samplesize];                        // n-th point
    memset( p, 0, sizeof(size_t) * c->samplesize );

    // Print the current max/min and freq
    FC_DrawAlign( scope->font, scope->render, 
                  r.x + r.w-1, r.y+1, FC_ALIGN_RIGHT, 
                  "Ymax=%.1f", c->max );
    FC_DrawAlign( scope->font, scope->render, 
                  r.x + r.w-1, r.y + r.h - 1 - FC_GetLineHeight( scope->font ), FC_ALIGN_RIGHT, 
                  "Ymin=%.1f", c->min );
    FC_Draw( scope->font, scope->render, r.x+1, r.y+1, "%.1f/s", c->afreq ); 

    /*int lead =c->length - cfifo_length( c->sampleBuffer );
    lead = lead < 0 ? 0 : lead;*/

    for( size_t i =0; i < c->length; i++ ) {

        // Horizontal position within the given clip rectangle
        float x =((float)(i) - c->hshift)*xStep + (float)r.x;

        // If multisampling is enabled, we need to read multiple doubles for each index i
        // the 'samplesize' property of the channel dictates how many values make up one sample
        for( int ms =0; ms < c->samplesize; ms++ ) {
            
            double val =cfifo_read( c->sampleBuffer, c->samplesize * i + ms );

            // Translate val to an y-value in the graph
            float y =yMid - fmax( c->min, fmin( c->max, val ) ) * yStep;

            // Draw...
            if( (int)x >= r.x) {
                //SDL_RenderDrawPoint( render, x, y );
                SDL_Point pnt = {x,y};
                points[ms][p[ms]++] = pnt;
            }

            // Adjust min and max, but try to avoid one single spike throwing the image off
            c->avg =c->avg * (1.-spread) + val * spread;
            
            if( c->vrange == SCOPE_VRANGE_AUTO_FIT ) {
                max =fmax( max, val );
                min =fmin( min, val );
            }
            else if( c->vrange == SCOPE_VRANGE_AUTO_OPTIMAL ) {
                if( val != 0 && val > max ) {
                    double f = fmin(1.0, (fabs(c->avg) / fabs(val)));// * damp;
                    f *=f*f;
                    max =max * (1.-f) + val * f;
                }
                else if( val != 0 && val < min ) {
                    double f = fmin(1.0, (fabs(c->avg) / fabs(val)));// * damp;
                    f *=f*f;
                    min =min * (1.-f) + val * f;
                }
            }
        }
    }

    for( int i =0; i < c->samplesize; i++ ) {
        setSDLPaletteColor( scope, i+1 );
        if( c->style == SCOPE_DRAW_LINES )
            SDL_RenderDrawLines( scope->render, points[i], p[i] );
        else
            SDL_RenderDrawPoints( scope->render, points[i], p[i] );
    }

    if( c->vrange == SCOPE_VRANGE_AUTO_FIT || c->vrange == SCOPE_VRANGE_AUTO_OPTIMAL ) {
        // Auto-zoom with damping/interpolation, factor in freq
        double damp =scope->freq * 0.0005;
        c->max =c->max * (1.-damp) + max * damp;
        c->min =c->min * (1.-damp) + min * damp;
    }
}


void
scope_updateDrawingArea( scope_t* scope, SDL_Rect area ) {
    // Window size or draw area has changed, we need to update the backdrop texture
    if( !SDL_RECT_EQ( area, scope->area ) || scope->backdrop == NULL ) {

        // Render the backdrop to a texture
        if( scope->backdrop != NULL )
            SDL_DestroyTexture( scope->backdrop );

        scope->backdrop = SDL_CreateTexture( scope->render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, area.w, area.h );
        SDL_SetRenderTarget( scope->render, scope->backdrop );
        SDL_SetRenderDrawColor( scope->render, 0x00, 0x00, 0x00, 0x00 );
        SDL_RenderClear( scope->render );
        
        // First, calculate the positions of the channels
        int pad =10;

        int height = (area.h - pad) / scope->nchannels - pad;
        int width = area.w - 2*pad;
        int x =pad + area.x, y =pad + area.y;
        for( int i =0; i < scope->nchannels; i++ ) {
            //if( scope->channel[i].mode == SCOPE_MODE_NONE ) continue; 
            // Absolute position/size within the render area
            SDL_Rect r = {x, y, width, height};
            scope->channel[i].area =r;

            // Render the frame and the grid for this channel to the texture
            setSDLPaletteColor( scope, 0 );
            scope_drawGrid( scope->render, r, 8 );
            SDL_RenderDrawRect( scope->render, (SDL_Rect*)&r );
        //    if( (int)yMid < r.y+r.h-10 && (int)yMid > r.y+10 )
          //      SDL_RenderDrawLine( render, r.x, yMid, r.x+r.w, yMid );
    
            // Render the static text
            FC_DrawAlign( scope->font, scope->render, x + area.w/2, y+1, FC_ALIGN_CENTER, "Channel %d", i+1 );

            y += height + pad;
        }
        scope->area =area;
        
        SDL_SetRenderTarget( scope->render, NULL );
    }

}

void
scope_updateChannel( scope_t* scope, unsigned int chan ) {
    schannel_t* c =&scope->channel[chan];
    if( c->mode == SCOPE_MODE_NONE ) return;

    if( c->atime == 0 ) c->atime = scope->time;

    // Compute the amount of samples that the view has shifted since the last draw
    c->hshift += (float)scope->freq * scope->elapsed;
    // Whole number of new samples 
    int delta  = (int)c->hshift;
    // Decimal part
    c->hshift -= delta;

       
    int i;
    for( i = 0; i < delta; i++ ) {
        double s =0.;
        // Consume the sample from the appropriate buffer/fifo
        if( c->mode == SCOPE_MODE_STREAM ) {
            // The queue may be empty. In that case we abort and notify the user about the synchronization loss
            if( atomq_estimateLength( c->sampleQueue ) < c->samplesize )
                break;         
        }
        for( int j =0; j < c->samplesize; j++ ) {
            if( c->mode == SCOPE_MODE_STATIC ) {
                s = sampler_getNext( c->sampler );
            } else if( c->mode == SCOPE_MODE_STREAM ) {
                atomq_dequeue( c->sampleQueue, &s );
            }

            // samplerBuffer will be used to draw the plot
            cfifo_push( c->sampleBuffer, s );
            if( cfifo_length( c->sampleBuffer ) > c->length * c->samplesize )
                cfifo_take( c->sampleBuffer );
        }
    }

    // Compute the actual frequency
    c->asamples += i;
    if( c->asamples > scope->freq ) {
        c->afreq    = (float)c->asamples / ((float)(scope->time - c->atime) / scope->ticksPerSec);
        c->asamples = 0;
        c->atime    = scope->time;
    }
}

void
scope_computeElapsedTime( scope_t* scope ) {
    uint64_t now = SDL_GetPerformanceCounter();
    if( scope->time == 0UL ) {
        scope->elapsed =0.f;
    } else {
        scope->elapsed =(double)(now - scope->time) / scope->ticksPerSec;
    }

    scope->time =now;
}

void
scope_update( scope_t* scope ) {
    // Compute the time between this and the last call 
    scope_computeElapsedTime( scope );
    
    // First we update every channel based on the elapsed time between calls to scope_update()
    for( int i =0; i < scope->nchannels; i++ )
        scope_updateChannel( scope, i );
    
    // Draw the backdrop texture
    if( scope->backdrop != NULL )
        SDL_RenderCopy( scope->render, scope->backdrop, NULL, &scope->area );

    // Draw the channels
    for( int i =0; i < scope->nchannels; i++ )
        scope_drawChannel( scope, i );


}
