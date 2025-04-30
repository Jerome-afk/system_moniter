#ifndef HEADER_H
#define HEADER_H

#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <cmath>
#include <regex>
#include <array>
#include <set>

// ImGui includes
#include "imgui/lib/imgui.h"
#include "imgui/lib/backend/imgui_impl_sdl.h"
#include "imgui/lib/backend/imgui_impl_opengl3.h"

// Forward declarations
class SystemInfo;
class MemoryInfo;
class NetworkInfo;

// Data structures
struct CPUData {
    float usagePercent = 0.0f;
    std::vector<float> history;
    bool paused = false;
    int fps = 60;
    float scale = 100.0f;
};

struct FanData {
    bool enabled = false;
    int speed = 0;
    int level = 0;
    std::vector<float> history;
    bool paused = false;
    int fps = 60;
    float scale = 100.0f;
};

struct ThermalData {
    float temperature = 0.0f;
    std::vector<float> history;
    bool paused = false;
    int fps = 60;
    float scale = 100.0f;
};

struct MemoryData {
    unsigned long total = 0;
    unsigned long used = 0;
    unsigned long free = 0;
    float usedPercent = 0.0f;
};

struct SwapData {
    unsigned long total = 0;
    unsigned long used = 0;
    unsigned long free = 0;
    float usedPercent = 0.0f;
};

struct DiskData {
    unsigned long total = 0;
    unsigned long used = 0;
    unsigned long free = 0;
    float usedPercent = 0.0f;
};

struct ProcessInfo {
    int pid;
    std::string name;
    std::string state;
    float cpuUsage;
    float memoryUsage;
    bool selected;
};

struct NetworkInterface {
    std::string name;
    std::string ipv4;
    
    // RX stats
    unsigned long rx_bytes;
    unsigned long rx_packets;
    unsigned long rx_errs;
    unsigned long rx_drop;
    unsigned long rx_fifo;
    unsigned long rx_frame;
    unsigned long rx_compressed;
    unsigned long rx_multicast;
    
    // TX stats
    unsigned long tx_bytes;
    unsigned long tx_packets;
    unsigned long tx_errs;
    unsigned long tx_drop;
    unsigned long tx_fifo;
    unsigned long tx_colls;
    unsigned long tx_carrier;
    unsigned long tx_compressed;
    
    float rx_usage_percent;
    float tx_usage_percent;
};

struct SystemTasks {
    int running = 0;
    int sleeping = 0;
    int uninterruptible = 0;
    int zombie = 0;
    int traced_stopped = 0;
    int interrupted = 0;
    int total = 0;
};

// System Info Class
class SystemInfo {
public:
    SystemInfo();
    ~SystemInfo();
    
    void update();
    
    // System info getters
    std::string getOSType() const;
    std::string getUser() const;
    std::string getHostname() const;
    std::string getCPUType() const;
    SystemTasks getTaskStats() const;
    
    // CPU, Fan, and Thermal data
    CPUData& getCPUData();
    FanData& getFanData();
    ThermalData& getThermalData();
    
    // Draw system monitor tab
    void drawSystemMonitor();
    
private:
    std::string osType;
    std::string user;
    std::string hostname;
    std::string cpuType;
    SystemTasks tasks;
    
    CPUData cpuData;
    FanData fanData;
    ThermalData thermalData;
    
    // CPU measurements
    unsigned long long lastTotalUser = 0;
    unsigned long long lastTotalUserLow = 0;
    unsigned long long lastTotalSys = 0;
    unsigned long long lastTotalIdle = 0;
    
    std::chrono::time_point<std::chrono::steady_clock> lastCpuUpdateTime;
    std::chrono::time_point<std::chrono::steady_clock> lastFanUpdateTime;
    std::chrono::time_point<std::chrono::steady_clock> lastThermalUpdateTime;
    
    // Private methods
    void updateSystemInfo();
    void updateCPUUsage();
    void updateFanInfo();
    void updateThermalInfo();
    void updateTaskStats();
    
    void drawPerformanceGraph(const char* label, std::vector<float>& history, float currentValue, 
                             bool& paused, int& fps, float& scale, const char* overlay);
};

// Memory Info Class
class MemoryInfo {
public:
    MemoryInfo();
    ~MemoryInfo();
    
    void update();
    std::vector<ProcessInfo>& getProcesses();
    
    MemoryData& getRAMData();
    SwapData& getSwapData();
    DiskData& getDiskData();
    
    // Draw memory monitor tab
    void drawMemoryMonitor();
    
private:
    MemoryData ram;
    SwapData swap;
    DiskData disk;
    std::vector<ProcessInfo> processes;
    std::string filterText;
    
    // Process CPU usage calculation
    std::unordered_map<int, unsigned long long> lastProcessCpuTime;
    std::unordered_map<int, std::chrono::time_point<std::chrono::steady_clock>> lastProcessUpdateTime;
    
    void updateMemoryInfo();
    void updateSwapInfo();
    void updateDiskInfo();
    void updateProcesses();
    
    float calculateProcessCpuUsage(int pid, unsigned long long processCpuTime);
};

// Network Info Class
class NetworkInfo {
public:
    NetworkInfo();
    ~NetworkInfo();
    
    void update();
    std::vector<NetworkInterface>& getInterfaces();
    
    // Draw network monitor tab
    void drawNetworkMonitor();
    
private:
    std::vector<NetworkInterface> interfaces;
    int currentTabIndex;
    
    void updateNetworkInterfaces();
    std::string formatBytes(unsigned long bytes);
};

#endif // HEADER_H
