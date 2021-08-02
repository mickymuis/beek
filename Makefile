CC = gcc
SDL = sdl2-config
CFLAGS = -Wall -g -O2 --std=c11 `$(SDL) --cflags`
LDFLAGS = -lpthread -lm `$(SDL) --libs` -lSDL2_ttf

DIRS = build build/stages build/SDL_FontCache bin
EXEC = bin/beek
OBJS = build/main.o build/window.o build/sampler.o build/cfifo.o build/atomq.o build/scope.o \
       build/flow.o build/stage.o \
       build/gaussian.o \
       build/stages/passthrough.o build/stages/lowpass.o build/stages/source.o build/stages/sink.o build/stages/scalespace.o\
       build/SDL_FontCache/SDL_FontCache.o
SOURCES = src/main.c src/window.c src/sampler.c src/cfifo.c src/atomq.c src/scope.c \
	  src/flow.c src/stage.c \
	  src/gaussian.c \
	  src/stages/passthrough.c src/stages/lowpass.c src/stages/source.c src/stages/sink.c src/stage/scalespace.c \
	  src/SDL_FontCache/SDL_FontCache.c
HEADERS = src/window.h src/sampler.h src/cfifo.h src/atomq.h src/scope.h \
	  src/flow.h src/stage.h src/stage_private.h \
	  src/gaussian.h \
	  src/stages/passthrough.h src/stages/lowpass.h src/stages/source.h src/stages/sink.h src/stages/scalespace.h\
	  src/SDL_FontCache/SDL_FontCache.h

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c $(HEADERS) $(DIRS)
	$(CC) $(CFLAGS) -c $< -o $@ 

build/flw/%.o: src/flw/%.c $(HEADERS) $(DIRS)
	$(CC) $(CFLAGS) -c $< -o $@ 

$(DIRS):
	mkdir -p $@

clean:
	rm -rf $(DIRS)

