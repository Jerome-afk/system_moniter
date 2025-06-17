CXX = g++
CC = gcc
EXE = system-monitor.exe

# Source files
SOURCES = main.cpp mem.cpp network.cpp system.cpp \
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_sdl.cpp imgui/backends/imgui_impl_opengl3.cpp \
    imgui/misc/gl3w/GL/gl3w.c

OBJS = $(SOURCES:.cpp=.o)
OBJS := $(OBJS:.c=.o)

# Compiler flags
CXXFLAGS = -std=c++17 -I. -Iimgui -Iimgui/backends -Iimgui/misc/gl3w -Iimgui/misc/sdl/include -Iimgui/misc/glfw/include -g -Wall
CFLAGS = $(CXXFLAGS)

# Libraries
LIBS = -Limgui/misc/sdl/lib -lSDL2 -lopengl32 -lgdi32 -limm32 -luser32 -lkernel32 -lws2_32 -lPdh -liphlpapi

all: $(EXE)
	@echo Build complete

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q /S *.o
	del /Q /S imgui\*.o
	del /Q /S imgui\backends\*.o
	del /Q /S imgui\misc\gl3w\*.o
	del /Q system-monitor.exe