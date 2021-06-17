CC = gcc
SDL = sdl2-config
CFLAGS = -Wall -g -O2 --std=c11 `$(SDL) --cflags`
LDFLAGS = -lpthread -lm `$(SDL) --libs`

DIRS = build build/dsp bin
EXEC = bin/a.out
OBJS = build/main.o build/window.o build/sampler.o build/adt.o build/scope.o build/dsp.o build/dsp/passthrough.o
SOURCES = src/main.c src/window.c src/sampler.c src/adt.c src/scope.c src/dsp.c src/dsp/passthrough.c
HEADERS = src/window.h src/sampler.h src/adt.h src/scope.h src/dsp.h src/dsp/passthrough.h

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c $(HEADERS) $(DIRS)
	$(CC) $(CFLAGS) -c $< -o $@ 

build/dsp/%.o: src/dsp/%.c $(HEADERS) $(DIRS)
	$(CC) $(CFLAGS) -c $< -o $@ 

$(DIRS):
	mkdir -p $@

clean:
	rm -rf $(DIRS)

