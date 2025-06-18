#include "header.h"

// SystemManager Implementation
SystemManager::SystemManager() {
    cpu_history.reserve(HISTORY_SIZE);
    fan_history.reserve(HISTORY_SIZE);
    temp_history.reserve(HISTORY_SIZE);
}

const int SystemManager::HISTORY_SIZE;

void SystemManager::Initialize() {
#ifdef _WIN32
    InitializeWindows();
#else
    InitializeLinux();
#endif
}

void SystemManager::Update() {
#ifdef _WIN32
    UpdateWindows();
#else
    UpdateLinux();
#endif

    // Update history
    if (g_monitor.GetAnimateGraphs()) {
        cpu_history.push_back(system_info.cpu_usage);
        fan_history.push_back((float)system_info.fan_speed);
        temp_history.push_back(system_info.temperature);
        
        if (cpu_history.size() > HISTORY_SIZE * 2) {
            cpu_history.erase(cpu_history.begin(), cpu_history.begin() + (cpu_history.size() - HISTORY_SIZE));
            fan_history.erase(fan_history.begin());
            temp_history.erase(temp_history.begin());
        }
    }
}

#ifdef _WIN32
void SystemManager::InitializeWindows() {
    // Get OS info
    system_info.os_type = "Windows";
    
    // Get username
    char username[256];
    DWORD username_len = sizeof(username);
    GetUserNameA(username, &username_len);
    system_info.username = username;
    
    // Get hostname
    char hostname[256];
    DWORD hostname_len = sizeof(hostname);
    GetComputerNameA(hostname, &hostname_len);
    system_info.hostname = hostname;
    
    // Get CPU info
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char cpu_name[256];
        DWORD cpu_name_size = sizeof(cpu_name);
        RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)cpu_name, &cpu_name_size);
        system_info.cpu_type = cpu_name;
        RegCloseKey(hKey);
    }
}

void SystemManager::UpdateWindows() {
    // Update CPU usage
    static PDH_HQUERY cpuQuery;
    static PDH_HCOUNTER cpuTotal;
    static bool initialized = false;
    
    if (!initialized) {
        PdhOpenQuery(NULL, NULL, &cpuQuery);
        PdhAddEnglishCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
        PdhCollectQueryData(cpuQuery);
        initialized = true;
    }
    
    PdhCollectQueryData(cpuQuery);
    PDH_FMT_COUNTERVALUE counterVal;
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    system_info.cpu_usage = (float)counterVal.doubleValue;
    
    // Update memory info
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    system_info.total_memory = memInfo.ullTotalPhys;
    system_info.used_memory = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    system_info.memory_usage = (float)(system_info.used_memory * 100.0 / system_info.total_memory);
    
    // Update process count
    DWORD processes[1024], cbNeeded;
    if (EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
        system_info.total_processes = cbNeeded / sizeof(DWORD);
    }
}
#else
void SystemManager::InitializeLinux() {
    // Get OS info
    struct utsname unameData;
    uname(&unameData);
    system_info.os_type = std::string(unameData.sysname) + " " + unameData.release;
    
    // Get username
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        system_info.username = pw->pw_name;
    }
    
    // Get hostname
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    system_info.hostname = hostname;
    
    // Get CPU info
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                system_info.cpu_type = line.substr(pos + 2);
                break;
            }
        }
    }
}

void SystemManager::UpdateLinux() {
    UpdateCPUUsage();
    UpdateThermalInfo();
}

void SystemManager::UpdateCPUUsage() {
    static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
    
    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);
    
    std::istringstream iss(line);
    std::string cpu;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    
    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
    
    unsigned long long totalUser = user + nice;
    unsigned long long totalSys = system + irq + softirq;
    unsigned long long totalIdle = idle + iowait;
    unsigned long long total = totalUser + totalSys + totalIdle;
    
    if (lastTotalUser != 0) {
        unsigned long long totalDiff = total - (lastTotalUser + lastTotalSys + lastTotalIdle);
        unsigned long long idleDiff = totalIdle - lastTotalIdle;
        
        system_info.cpu_usage = (float)((totalDiff - idleDiff) * 100.0 / totalDiff);
    }
    
    lastTotalUser = totalUser;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;
}

void SystemManager::UpdateThermalInfo() {
    // Try to read temperature from thermal zones
    std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    if (temp_file.is_open()) {
        int temp_millicelsius;
        temp_file >> temp_millicelsius;
        system_info.temperature = temp_millicelsius / 1000.0f;
    }
    
    // Try to read fan info (simplified)
    std::ifstream fan_file("/sys/class/hwmon/hwmon1/fan1_input");
    if (fan_file.is_open()) {
        fan_file >> system_info.fan_speed;
        system_info.fan_active = system_info.fan_speed > 0;
    }
}
#endif

void SystemManager::RenderSystemInfo() {
    // Basic system information
    ImGui::Text("Operating System: %s", system_info.os_type.c_str());
    ImGui::Text("User: %s", system_info.username.c_str());
    ImGui::Text("Hostname: %s", system_info.hostname.c_str());
    ImGui::Text("Total Processes: %d", system_info.total_processes);
    ImGui::Text("Running: %d, Sleeping: %d, Zombie: %d, Stopped: %d", 
               system_info.running_processes, system_info.sleeping_processes,
               system_info.zombie_processes, system_info.stopped_processes);
    ImGui::Text("CPU: %s", system_info.cpu_type.c_str());
    
    ImGui::Separator();
    
    // Performance tabs
    if (ImGui::BeginTabBar("PerformanceTabs")) {
        if (ImGui::BeginTabItem("CPU")) {
            RenderCPUTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Fan")) {
            RenderFanTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Thermal")) {
            RenderThermalTab();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}

void SystemManager::RenderCPUTab() {
    ImGui::Text("CPU Usage: %.1f%%", system_info.cpu_usage);
    
    RenderGraphControls();
    
    if (!cpu_history.empty()) {
        static int display_start = 0;

        if (g_monitor.GetAnimateGraphs()) {
            display_start = std::max(0, (int)cpu_history.size() - HISTORY_SIZE);
        }

        int display_count = std::min(HISTORY_SIZE, (int)cpu_history.size() - display_start);

        ImGui::PlotLines("CPU Usage", cpu_history.data() + display_start, display_count, 
                       0, nullptr, 0.0f, g_monitor.GetGraphYScale(), ImVec2(0, 200));
        
        // Overlay text
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 50);
        ImGui::Text("Current: %.1f%%", system_info.cpu_usage);
    }
}

void SystemManager::RenderFanTab() {
    ImGui::Text("Fan Status: %s", system_info.fan_active ? "Active" : "Inactive");
    ImGui::Text("Fan Speed: %d RPM", system_info.fan_speed);
    
    RenderGraphControls();
    
    if (!fan_history.empty()) {
        float max_speed = *std::max_element(fan_history.begin(), fan_history.end());
        ImGui::PlotLines("Fan Speed", fan_history.data(), fan_history.size(), 
                       0, nullptr, 0.0f, max_speed, ImVec2(0, 200));
    }
}

void SystemManager::RenderThermalTab() {
    ImGui::Text("Temperature: %.1f°C", system_info.temperature);
    
    RenderGraphControls();
    
    if (!temp_history.empty()) {
        ImGui::PlotLines("Temperature", temp_history.data(), temp_history.size(), 
                       0, nullptr, 0.0f, 100.0f, ImVec2(0, 200));
        
        // Overlay text
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 50);
        ImGui::Text("Current: %.1f°C", system_info.temperature);
    }
}

void SystemManager::RenderGraphControls() {
    bool animate = g_monitor.GetAnimateGraphs();
    if (ImGui::Checkbox("Animate", &animate)) {
        g_monitor.SetAnimateGraphs(animate);
    }
    
    float fps = g_monitor.GetGraphFPS();
    if (ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f)) {
        g_monitor.SetGraphFPS(fps);
    }
    
    float scale = g_monitor.GetGraphYScale();
    if (ImGui::SliderFloat("Y Scale", &scale, 50.0f, 200.0f)) {
        g_monitor.SetGraphYScale(scale);
    }
}

// SystemMonitor Implementation
SystemMonitor::SystemMonitor() : memory_manager(&const_cast<SystemInfo&>(system_manager.GetSystemInfo())) {
    system_manager.Initialize();
}

void SystemMonitor::Update() {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    system_manager.Update();
    memory_manager.Update();
    network_manager.Update();
}

void SystemMonitor::RenderSystemMonitor() {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("System Monitor")) {
            system_manager.RenderSystemInfo();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Memory & Processes")) {
            memory_manager.RenderMemoryAndProcesses();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Network")) {
            network_manager.RenderNetwork();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}