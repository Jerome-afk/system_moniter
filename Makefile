# Simple Makefile for system monitor
# This version works without SDL2 for console-based monitoring

CXX = g++
CC = gcc
EXE = system-monitor
IMGUI_DIR = imgui/lib/

# Source files
SOURCES = main_console.cpp
SOURCES += system.cpp
SOURCES += mem.cpp
SOURCES += network.cpp

# Object files
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

# Compiler flags
CXXFLAGS = -std=c++11 -I. -g -Wall -Wformat
CFLAGS = $(CXXFLAGS)

# Libraries
LIBS = -lpthread

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backend/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:imgui/lib/gl3w/GL/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:imgui/lib/glad/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)