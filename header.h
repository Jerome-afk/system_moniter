#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <imgui.h>
#include <SDL2/SDL.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <GL/gl3w.h>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <iphlpapi.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <pwd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/statvfs.h>
#include <signal.h>
#include <unistd.h>
#endif

// Data structures
struct ProcessInfo {
    int pid;
    std::string name;
    std::string state;
    float cpu_usage;
    float memory_usage;
};

struct NetworkInterface {
    std::string name;
    std::string description;
    std::string ipv4;
    int type;
    std::string ipv6;
    bool operational_status;
    std::string mac_address;
    uint32_t speed_mbps;
    uint64_t rx_rate, rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
    uint64_t tx_rate, tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
    uint64_t rx_packet_rate, tx_packet_rate;
};

struct SystemInfo {
    std::string os_type;
    std::string username;
    std::string hostname;
    int total_processes = 0;
    int running_processes = 0;
    int sleeping_processes = 0;
    int zombie_processes = 0;
    int stopped_processes = 0;
    std::string cpu_type;
    float cpu_usage = 0.0f;
    float memory_usage = 0.0f;
    float swap_usage = 0.0f;
    float disk_usage = 0.0f;
    float temperature = 0.0f;
    int fan_speed = 0;
    bool fan_active = false;
    uint64_t total_memory = 0;
    uint64_t used_memory = 0;
    uint64_t total_swap = 0;
    uint64_t used_swap = 0;
    uint64_t total_disk = 0;
    uint64_t used_disk = 0;
};

// Forward declarations
class SystemMonitor;
class SystemManager;
class MemoryManager;
class NetworkManager;

// System Manager Class
class SystemManager {
private:
    SystemInfo system_info;
    std::vector<float> cpu_history;
    std::vector<float> fan_history;
    std::vector<float> temp_history;
    static const int HISTORY_SIZE = 200;

public:
    SystemManager();
    void Initialize();
    void Update();
    void UpdateCPUUsage();
    void UpdateThermalInfo();
    
    // Getters
    const SystemInfo& GetSystemInfo() const { return system_info; }
    const std::vector<float>& GetCPUHistory() const { return cpu_history; }
    const std::vector<float>& GetFanHistory() const { return fan_history; }
    const std::vector<float>& GetTempHistory() const { return temp_history; }
    
    // Rendering
    void RenderSystemInfo();
    void RenderCPUTab();
    void RenderFanTab();
    void RenderThermalTab();
    void RenderGraphControls();

private:
#ifdef _WIN32
    void InitializeWindows();
    void UpdateWindows();
#else
    void InitializeLinux();
    void UpdateLinux();
#endif
};

// Memory Manager Class
class MemoryManager {
private:
    std::vector<ProcessInfo> processes;
    std::vector<bool> selected_processes;
    char process_filter[256] = "";
    SystemInfo* system_info_ref;

public:
    MemoryManager(SystemInfo* sys_info);
    void Update();
    void UpdateMemoryInfo();
    void UpdateProcesses();
    void UpdateDiskInfo();
    void KillSelectedProcesses();
    
    // Getters
    const std::vector<ProcessInfo>& GetProcesses() const { return processes; }
    
    // Rendering
    void RenderMemoryAndProcesses();
    
    // Utility
    std::string FormatBytes(uint64_t bytes);
};

// Network Manager Class
class NetworkManager {
private:
    std::vector<NetworkInterface> network_interfaces;
    std::vector<NetworkInterface> previous_interfaces;
    std::chrono::steady_clock::time_point previous_update_time;

public:
    NetworkManager();
    void Update();
    // void UpdateNetworkInfo();
    void UpdateNetworkInterfaces();
    void CalculateNetworkRates();

    #ifdef _WIN32
    void UpdateNetworkInterfacesWindows();
    #else
    void UpdateNetworkInterfacesLinux();
    void GetInterfaceDetails(NetworkInterface&);
    #endif

    
    // Getters
    const std::vector<NetworkInterface>& GetNetworkInterfaces() const { return network_interfaces; }
    
    // Rendering
    void RenderNetwork();
    void RenderNetworkTable(bool is_rx);
    void RenderNetworkInfo();
    void RenderNetworkStatistics();
    
    // Utility
    std::string FormatRate(uint64_t bytes_per_sec);
    std::string FormatBytes(uint64_t bytes);
};

// Main System Monitor Class
class SystemMonitor {
private:
    SystemManager system_manager;
    MemoryManager memory_manager;
    NetworkManager network_manager;
    std::mutex data_mutex;
    
    // UI State
    bool animate_graphs = true;
    float graph_fps = 30.0f;
    float graph_y_scale = 100.0f;
    int selected_tab = 0;

public:
    SystemMonitor();
    void Update();
    void RenderSystemMonitor();
    
    // Getters for managers
    SystemManager& GetSystemManager() { return system_manager; }
    MemoryManager& GetMemoryManager() { return memory_manager; }
    NetworkManager& GetNetworkManager() { return network_manager; }
    std::mutex& GetDataMutex() { return data_mutex; }
    
    // UI State getters/setters
    bool GetAnimateGraphs() const { return animate_graphs; }
    void SetAnimateGraphs(bool animate) { animate_graphs = animate; }
    float GetGraphFPS() const { return graph_fps; }
    void SetGraphFPS(float fps) { graph_fps = fps; }
    float GetGraphYScale() const { return graph_y_scale; }
    void SetGraphYScale(float scale) { graph_y_scale = scale; }
};

// Global variables
extern SystemMonitor g_monitor;
extern bool g_running;

// Function declarations
void UpdateThread();

#endif // SYSTEM_MONITOR_H