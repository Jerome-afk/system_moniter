# Cross-Platform System Monitor

A comprehensive system monitoring application built with C++ and Dear ImGui that provides real-time system resource monitoring for both Linux and Windows platforms.

## Features

### System Monitor Tab
- **System Information**: OS type, username, hostname, CPU model
- **Process Statistics**: Total, running, sleeping, zombie, and stopped processes
- **Performance Graphs**: Real-time CPU, Fan, and Thermal monitoring with:
  - Animated graphs with pause/resume functionality
  - Adjustable FPS control (1-60 FPS)
  - Configurable Y-axis scaling
  - Overlay text showing current values

### Memory & Processes Tab
- **Memory Usage**: Visual progress bars for RAM, SWAP, and Disk usage
- **Process Table**: Detailed process information including:
  - PID (Process ID)
  - Process name
  - Current state
  - CPU usage percentage
  - Memory usage percentage
- **Process Filtering**: Real-time text-based filtering
- **Multi-selection**: Select multiple processes in the table

### Network Tab
- **Network Interfaces**: List all network interfaces with IPv4 addresses
- **Detailed Statistics**: Separate RX/TX tables showing:
  - Bytes, packets, errors, drops
  - FIFO, frame/collisions, compressed data
  - Multicast (RX) and carrier (TX) information
- **Visual Usage**: Progress bars showing network usage (0GB-2GB scale)
- **Smart Formatting**: Automatic conversion between bytes, KB, MB, GB

## Dependencies

### Required Libraries
- **Dear ImGui**: GUI framework
- **GLFW3**: Window management and OpenGL context
- **OpenGL 3.3+**: Graphics rendering
- **GL3W**: OpenGL function loader

### Platform-Specific Dependencies

#### Linux
- Standard system libraries (no additional dependencies)
- Reads from `/proc` filesystem for system information
- Supports thermal zones and hardware monitoring

#### Windows
- PDH (Performance Data Helper) library
- IPHLPAPI for network information
- PSAPI for process information
- Windows Registry for CPU information

## Building the Project

### Prerequisites

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libglfw3-dev libgl1-mesa-dev
```

#### Linux (Fedora/CentOS)
```bash
sudo dnf install gcc-c++ cmake glfw-devel mesa-libGL-devel
```

#### Windows
- Visual Studio 2019 or later
- CMake 3.10+
- vcpkg (recommended for dependencies)

### Download Dependencies

1. **Clone Dear ImGui**:
```bash
git clone https://github.com/ocornut/imgui.git
```

2. **Download GL3W**:
```bash
# Visit https://github.com/skaslev/gl3w and download
# Or use the generated files from the imgui examples
```

### Project Structure
```
SystemMonitor/
├── main.cpp                 # Main application code
├── CMakeLists.txt          # Build configuration
├── imgui/                  # Dear ImGui source
│   ├── imgui.cpp
│   ├── imgui_*.cpp
│   └── backends/
│       ├── imgui_impl_glfw.cpp
│       └── imgui_impl_opengl3.cpp
└── gl3w/                   # OpenGL loader
    └── GL/
        └── gl3w.c
```

### Build Commands

```bash
mkdir build
cd build
cmake ..
make  # Linux
# or
cmake --build . --config Release  # Windows
```

## Usage

1. **Launch the application**:
```bash
./SystemMonitor  # Linux
SystemMonitor.exe  # Windows
```

2. **Navigate between tabs** to view different system information
3. **Customize graphs** using the control sliders and animation toggle
4. **Filter processes** using the search box in the Memory & Processes tab
5. **Select multiple processes** by clicking on rows in the process table

## Implementation Details

### Cross-Platform Compatibility
The application uses conditional compilation (`#ifdef _WIN32`) to provide platform-specific implementations:

- **Linux**: Reads from `/proc` filesystem, `/sys` for thermal data
- **Windows**: Uses Windows APIs (PDH, WMI, Registry)

### Performance Optimization
- **Multi-threading**: Separate thread for data collection to maintain UI responsiveness
- **Efficient data structures**: Ring buffers for graph history
- **Minimal system calls**: Cached readings where appropriate

### Memory Management
- **RAII principles**: Automatic resource cleanup
- **STL containers**: Safe memory management with vectors and strings
- **Thread safety**: Mutex protection for shared data

## Extending the Monitor

### Adding New Metrics
1. Add new fields to `SystemInfo` struct
2. Implement platform-specific collection in `UpdateLinux()`/`UpdateWindows()`
3. Add UI rendering in appropriate tab function

### Adding New Platforms
1. Add platform detection in preprocessor directives
2. Implement platform-specific initialization and update functions
3. Add platform-specific dependencies to CMakeLists.txt

## Known Limitations

1. **Windows thermal monitoring**: Limited support for temperature sensors
2. **Process CPU usage**: Simplified calculation (can be enhanced)
3. **Network rate calculation**: Shows cumulative data (not per-second rates)
4. **Disk usage**: Basic implementation (can be expanded for multiple drives)

## Future Enhancements

- [ ] Real-time network throughput calculation
- [ ] GPU monitoring support
- [ ] Process kill/suspend functionality
- [ ] Configuration file support
- [ ] Alert system for resource thresholds
- [ ] Export data to CSV/JSON
- [ ] Plugin system for custom monitors

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add appropriate platform-specific implementations
4. Test on both Linux and Windows
5. Submit a pull request

## License

This project is provided as-is for educational and development purposes. Please ensure compliance with your organization's policies regarding system monitoring tools.