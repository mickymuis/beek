#include "timer.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>

struct tmr {
    thrd_t              thread;
    cnd_t               cond;
    atomic_bool         stop;
    _Atomic uint64_t    ticks;
    float               frequency;
    struct timespec     interval;
    bool                running;
};

static void
set_interval( struct timespec* ts, float freq ) {
    float ival    =1.f / freq;
    ts->tv_sec    =(int)ival;
    ts->tv_nsec   =(long)((float)10e9L * (ival - ts->tv_sec));
}

/** Timer thread function */
static int
tmr_main( tmr_t* t ) {
    struct timespec remain, ival;

    while( atomic_load( &t->stop ) == false ) {
        ival =t->interval;

        while(1) { 
            thrd_sleep( &ival, &remain );
            // Make sure we have slept the correct amount
            if( remain.tv_sec == 0 && remain.tv_nsec == 0 ) break;
            ival =remain;
        }

        // TODO: We should account for the inevitable clock drift here...

        atomic_fetch_add_explicit( &t->ticks, 1, memory_order_release );
        // Broadcast this clock tick
        cnd_broadcast( &t->cond );
    }
    return 0;
}

tmr_t*
tmr_create() {
    tmr_t* t     =malloc( sizeof( tmr_t ) );
    t->frequency =1.f;
    t->running   =false;
    cnd_init( &t->cond );
    return t;
}

void
tmr_destroy( tmr_t* t ) {
    assert( t != NULL );
    tmr_stop( t );
    cnd_destroy( &t->cond );
    free( t );
}

void
tmr_setFrequency( tmr_t* t, float freq ) {
    assert( t != NULL );
    t->frequency =freq;
}

void
tmr_wait( tmr_t* t, mtx_t* mutex ) {
    assert( t != NULL );
    if( !t->running ) return;
    ttick_t begin = atomic_load_explicit( &t->ticks, memory_order_acquire );

    while( atomic_load_explicit( &t->ticks, memory_order_acquire) == begin ) {
        cnd_wait( &t->cond, mutex );
        // There may be a spurious wake up, so we check if the timer has actually increased
    }
}

ttick_t
tmr_ticksElapsed( tmr_t* t ) {
    assert( t != NULL );
    return atomic_load_explicit( &t->ticks, memory_order_acquire );
}

bool
tmr_start( tmr_t* t ) {
    assert( t != NULL );
    if( t->running ) return false;
    atomic_store( &t->stop, false );
    atomic_store( &t->ticks, 0 );

    set_interval( &t->interval, t->frequency );

    return thrd_create( &t->thread, (thrd_start_t)tmr_main, (void*)t ) == thrd_success;
}

void
tmr_stop( tmr_t* t ) {
    assert( t != NULL );
    if( !t->running ) return;

    atomic_store( &t->stop, true );
    thrd_join( t->thread, NULL );
}
