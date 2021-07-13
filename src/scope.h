#ifndef SCOPE_H
#define SCOPE_H

#include <SDL2/SDL.h>

#include "sampler.h"
#include "cfifo.h"

static const size_t SCOPE_MAX_LENGTH   = 65535;
static const size_t SCOPE_FIFO_DEPTH   = 1024;

enum scope_channelSampleMode {
    SCOPE_MODE_NONE,
    SCOPE_MODE_STATIC,
    SCOPE_MODE_STREAM
};

enum scope_channelDrawStyle {
    SCOPE_DRAW_POINTS,
    SCOPE_DRAW_LINES
};

enum scope_channelVRange {
    SCOPE_VRANGE_FIXED,
    SCOPE_VRANGE_LOCKED,
    SCOPE_VRANGE_AUTO_FIT,
    SCOPE_VRANGE_AUTO_OPTIMAL
};

typedef struct scope_t scope_t;

/** Create a new scope with @nchannels number of channels, associated with rendering context @render.*/
scope_t*
scope_create( unsigned int nchannels, SDL_Renderer* render );

/** Destroy @scope and free all associated memory including the channels and buffers */
void
scope_destroy( scope_t* scope );

/** Sets the number of samples per second to @freq. 
    The view will move at this rate and scope_update() will consume samples at this rate */
void
scope_setFrequency( scope_t* scope, unsigned int freq );

/** Initialize channel @chan to @mode. Calls to this function causes the view to reset,
    and internal buffers are reallocated appropriately. */
void
scope_initChannel( scope_t* scope, unsigned int chan, int mode );

/** Tells the channel @chan how many consecutive elements in the buffer make up one sample (default is 1). */
void
scope_setChannelMultiSample( scope_t* scope, unsigned int chan, unsigned int sampleSize );

/** Sets the static buffer of channel @chan to be @sampler. @sampler position will be modified as scope may call sampler_getNext() on it.
    On scope channels that do not have mode equal to SCOPE_CHANNEL_STATIC this function will have no effect. */
void
scope_setChannelBuffer( scope_t* scope, unsigned int chan, sampler_t* sampler );

/** Adds @sample to the sample FIFO of channel @chan. 
    If samples are pushed at a higher rate than scope_update() consumes them, samples may get lost. 
    scope_setSpeed() sets this rate, while scope_update() must be called often enough to keep the internal FIFO from overflowing.
    On scope channels that do not have mode equal to SCOPE_CHANNEL_STREAM this function will have no effect. */
void
scope_pushChannelFifo( scope_t* scope, unsigned int chan, double sample );

/** Sets the channel's drawing style to either SCOPE_CHANNEL_POINTS or SCOPE_CHANNEL_LINES */
void
scope_setChannelDrawStyle( scope_t* scope, unsigned int chan, int style );

/** Sets channel @chan to display @length samples horizontally */
void
scope_setChannelLength( scope_t* scope, unsigned int chan, unsigned int length );

/** Sets the channel's fixed vertical range to [min;max] */
void
scope_setChannelVerticalRange( scope_t* scope, unsigned int chan, double min, double max );

/** Sets the channel's vertical range mode to @mode.
    SCOPE_VRANGE_AUTO_FIT always fits the entire vertical range of the data into the view
    SCOPE_VRANGE_AUTO_OPTIMAL tries to fit the vertical range such that it is less influenced by spikes and outliers */
void 
scope_setChannelVerticalMode( scope_t* scope, unsigned int chan, int mode );

/** Locks the verticale range of channel @chan to the range of @toChan, making it easier to visually compare the two views. 
    */
void
scope_lockChannelRange( scope_t* scope, unsigned int chan, unsigned int toChan );

/** Helper function that draws a scope grid that fits @r to @render with @vdiv vertical subdivisions */
void
scope_drawGrid( SDL_Renderer* render, SDL_Rect r, int vdiv );

/** Draw channel @chan. This function only draws the channel contents and does not sample new values into the buffer */
void
scope_drawChannel( scope_t* scope, unsigned int chan );

/** Sets the rectangle where @scope should be drawn. If @area is different from the previously stored rect,
    this function re-allocates and renders any static elements (such as grids).*/ 
void
scope_updateDrawingArea( scope_t* scope, SDL_Rect area );

/** Updates channel @chan without drawing it. */
void
scope_updateChannel( scope_t* scope, unsigned int chan );

/** Updates and redraws @scope. This function should be called from your main loop.
    scope_update() keeps track of the time that has elapsed since the last call to it and advances the view accordingly.
    Channels that have SCOPE_CHANNEL_STREAM set will also consume samples from the internal FIFO according to the elapsed time.
*/
void
scope_update( scope_t* scope );

#endif
