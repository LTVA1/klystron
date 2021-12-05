TARGET = ../bin/makebundle.exe

ifdef COMSPEC
SDL = -I /mingw/include/sdl `pkg-config --cflags sdl2`
else
SDL = `sdl2-config --cflags`
endif

$(TARGET): makebundle.c ../../src/util/bundle.h
	@mkdir -p ../bin
	gcc -o $(TARGET) -D_XOPEN_SOURCE makebundle.c -std=c99 -I ../../src $(SDL) -Wall -O3