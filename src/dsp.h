#ifndef DSP_H
#define DSP_H

typedef void(*dsp_consume_func_t)(void*);
typedef void(*dsp_destroy_func_t)(void*);

typedef struct {
    int inputs;
    int outputs;
    double in[4];
    double out[4];
    dsp_consume_func_t consume;
    dsp_destroy_func_t destroy;

} dsp_worker_t;

typedef struct {
    dsp_worker_t **workers;
    int workers_size;
    int nworkers; 
    int frequency;
} dsp_t;

dsp_t*
dsp_create();

void
dsp_destroy( dsp_t* dsp );

void
dsp_destroyWorker( dsp_worker_t* worker );

void
dsp_addWorker( dsp_t* dsp, dsp_worker_t* worker );

void
dsp_removeWorker( dsp_t* dsp, dsp_worker_t* worker );

#endif
