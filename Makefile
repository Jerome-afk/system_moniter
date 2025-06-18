# Detect OS
ifeq ($(OS),Windows_NT)
    PLATFORM = WINDOWS
    EXE = system-monitor.exe
    RM = del /Q /S
    SDL_LIB = -Limgui/misc/sdl/lib -lSDL2
    GL_LIBS = -lopengl32 -lgdi32 -limm32 -luser32 -lkernel32 -lws2_32 -lPdh -liphlpapi
else
    PLATFORM = LINUX
    EXE = system-monitor
    CONDA_PREFIX ?= /home/jerootieno/miniconda3/envs/opengl-env
    RM = rm -f
    SDL_LIB = -Limgui/misc/linux/sdl2-local/install/lib -lSDL2
    CONDA_SYSROOT := $(CONDA_PREFIX)/x86_64-conda-linux-gnu/sysroot

    # Only for OpenGL and pthread, use Conda sysroot paths explicitly
    GL_LIBS = -L$(CONDA_SYSROOT)/usr/lib64 -L$(CONDA_SYSROOT)/lib64 -lGL -ldl
endif

CXX = g++
CC = gcc

# Source files
SOURCES = main.cpp mem.cpp network.cpp system.cpp \
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_sdl.cpp imgui/backends/imgui_impl_opengl3.cpp \
    imgui/misc/gl3w/GL/gl3w.c

OBJS = $(SOURCES:.cpp=.o)
OBJS := $(OBJS:.c=.o)

# Compiler flags
CXXFLAGS = -std=c++17 -I. -Iimgui -Iimgui/backends -Iimgui/misc/gl3w -Iimgui/misc/sdl/include -g -Wall

CFLAGS = $(CXXFLAGS)

# Combined libs
LIBS = $(SDL_LIB) $(GL_LIBS)

# Targets
all: $(EXE)
	@echo Build complete on $(PLATFORM)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o
	$(RM) imgui/*.o
	$(RM) imgui/backends/*.o
	$(RM) imgui/misc/gl3w/*.o
	$(RM) $(EXE)