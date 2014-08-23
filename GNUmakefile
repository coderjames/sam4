# Note: This Makefile is intended for use with GNU Make

PROGRAM_NAME = sam

.PHONY: all clean $(PROGRAM_NAME)

INCLUDE_DIRS = C:/MinGW/msys/1.0/include

INCLUDE_STRING = $(patsubst %,-I%,$(INCLUDE_DIRS))

LIB_DIRS = C:/MinGW/msys/1.0/lib
LIB_NAMES = \
 	allegro            \
	allegro_main       \
	allegro_audio      \
	allegro_acodec     \
	allegro_image      \
	allegro_physfs     \
	allegro_primitives \
	allegro_font       \
	allegro_ttf        \
	physfs

LIB_STRING = $(patsubst %,-L%,$(LIB_DIRS)) $(patsubst %,-l%,$(LIB_NAMES))

CXX = g++
CXXFLAGS = -std=gnu++11 -Wall -Wextra -g -march=native -O1 $(INCLUDE_STRING)
LDFLAGS = $(LIB_STRING) 
#RM = del /F /Q
RM = rm

all: $(PROGRAM_NAME)

$(PROGRAM_NAME): $(PROGRAM_NAME).exe

$(PROGRAM_NAME).exe: main.o interactives.o level1.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.cpp level1.h interactives.hpp sam_shared.hpp

interactives.o: interactives.cpp interactives.hpp sam_shared.hpp level1.h

level1.o: level1.h

clean:
	$(RM) $(PROGRAM_NAME).exe *.o
