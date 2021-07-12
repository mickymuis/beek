#include <stdio.h>
#include "window.h"

void
window_init( window_t* win ) {
    win->handle =NULL;
    win->on_redraw =NULL;
    win->user =NULL;
}

int
window_create( window_t* win ) {

    SDL_Init( SDL_INIT_VIDEO );
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );
/*    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    glEnable(GL_MULTISAMPLE);*/
    
    // We use SDL2 to create a resizable window
    win->handle = SDL_CreateWindow(
            "Beek",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800,
            600,
            SDL_WINDOW_SHOWN );

    if( !win->handle) return -1;
    
    // Set-up a accelerated rendering context
    win->render = SDL_CreateRenderer(
        win->handle, -1, SDL_RENDERER_ACCELERATED /*| SDL_RENDERER_PRESENTVSYNC */ );
    if( !win->render ) {
        SDL_DestroyWindow( win->handle );
        SDL_Quit();
        return -1;
    }

    SDL_RenderClear( win->render );

    SDL_SetWindowResizable( win->handle, true );
    //SDL_RenderSetLogicalSize( win->render, 1024, 1024 );
    //SDL_RenderSetScale( win->render, .5, .5);

    return 0;
}

int
window_mainloop( window_t* win ) {
    SDL_Event e;
    bool quit =false;
    SDL_Rect rect = {0,0,0,0}; // Drawable area

    //SDL_RenderGetLogicalSize( win->render, &w, &h );
    SDL_GetWindowSize( win->handle, &rect.w, &rect.h );

    while( !quit ) {
        SDL_SetRenderDrawColor(win->render, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(win->render);

        if( win->on_redraw ) win->on_redraw( win, rect );

        SDL_RenderPresent(win->render);
        while( SDL_PollEvent(&e) ) {


            if( e.type == SDL_QUIT )
                quit =true;
            else if( e.type == SDL_KEYDOWN ) {
                switch( e.key.keysym.sym ) {
                    case SDLK_q:
                        quit =true;
                        break;
                    default: break;
                }
            }
            else if( e.type == SDL_WINDOWEVENT ) {
                if( e.window.event == SDL_WINDOWEVENT_RESIZED ) {
                    //SDL_RenderClear( win->render );
                    SDL_GetWindowSize( win->handle, &rect.w, &rect.h );
                }
            }
        }
        SDL_Delay( 32 );
    }

    return 0;
}

void
window_destroy( window_t* win ) {
    SDL_DestroyWindow( win->handle );

}
