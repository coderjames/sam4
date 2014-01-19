# Note: This Makefile is intended for use with GNU Make

PROGRAM_NAME = sam

.PHONY: all clean $(PROGRAM_NAME)

CXX = g++
CXXFLAGS = -Wall -O1
LDFLAGS = -lallegro -lallegro_main -lallegro_audio -lallegro_acodec -lallegro_image -lallegro_physfs -lallegro_primitives -lallegro_font -lallegro_ttf -lphysfs

all: $(PROGRAM_NAME)

$(PROGRAM_NAME): $(PROGRAM_NAME).exe

$(PROGRAM_NAME).exe: main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.cpp level1.h

clean:
	$(RM) $(PROGRAM_NAME).exe *.o
