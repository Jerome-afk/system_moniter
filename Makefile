# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I.
LDFLAGS = -lGL -ldl -lSDL2 -lpthread

# Source files
SOURCES = main.cpp system.cpp mem.cpp network.cpp \
          imgui/lib/imgui.cpp \
          imgui/lib/imgui_demo.cpp \
          imgui/lib/imgui_draw.cpp \
          imgui/lib/imgui_tables.cpp \
          imgui/lib/imgui_widgets.cpp \
          imgui/lib/backend/imgui_impl_sdl.cpp \
          imgui/lib/backend/imgui_impl_opengl3.cpp \
          imgui/lib/gl3w/GL/gl3w.c

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS := $(OBJECTS:.c=.o)

# Executable name
EXECUTABLE = system-monitor

# Default target
all: $(EXECUTABLE)

# Linking
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

# Compilation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

# Run the application
run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: all clean run
