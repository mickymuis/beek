#ifndef WINDOW_H
#define WINDOW_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct window_t window_t;

typedef void(*window_renderfunc_t)(window_t*, SDL_Rect);

struct window_t {
    SDL_Window* handle;
    SDL_Renderer* render;
    window_renderfunc_t on_redraw;
    void* user;
};

void
window_init( window_t* );

int
window_create( window_t* win );

int
window_mainloop( window_t* win );

void
window_destroy( window_t* win );

#endif
