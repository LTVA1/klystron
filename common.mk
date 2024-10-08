#undefine COMSPEC

# make it possible to do a verbose build by running `make V=1`
ifeq ($(V),1)
Q= 
MSG=@true
else
Q=@
MSG=@$(ECHO)
endif

# SDL include/lib flags to pass to the compiler
ifdef COMSPEC
	SDLFLAGS := `pkg-config --cflags sdl2` -lSDL2 -lwinmm
	SDLLIBS :=  -L C:/MinGW/lib -lSDL2main -lSDL2 -lSDL2_image -lwinmm
else
	SDLFLAGS := $(shell sdl2-config --cflags) -U_FORTIFY_SOURCE -lwinmm
	SDLLIBS := $(shell sdl2-config --libs) -lSDL2_image
endif

