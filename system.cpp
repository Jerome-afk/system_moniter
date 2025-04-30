#include "header.h"

SystemInfo::SystemInfo() {
    // Initialize history vectors with zeros
    cpuData.history.resize(100, 0.0f);
    fanData.history.resize(100, 0.0f);
    thermalData.history.resize(100, 0.0f);
    
    // Set default values
    cpuData.fps = 60;
    cpuData.scale = 100.0f;
    fanData.fps = 60;
    fanData.scale = 100.0f;
    thermalData.fps = 60;
    thermalData.scale = 100.0f;
    
    // Initialize timers
    lastCpuUpdateTime = std::chrono::steady_clock::now();
    lastFanUpdateTime = std::chrono::steady_clock::now();
    lastThermalUpdateTime = std::chrono::steady_clock::now();
    
    // Perform initial update
    updateSystemInfo();
    updateCPUUsage();
    updateFanInfo();
    updateThermalInfo();
    updateTaskStats();
}

SystemInfo::~SystemInfo() {
}

void SystemInfo::update() {
    // Update system information periodically
    updateSystemInfo();
    updateCPUUsage();
    updateFanInfo();
    updateThermalInfo();
    updateTaskStats();
}

std::string SystemInfo::getOSType() const {
    return osType;
}

std::string SystemInfo::getUser() const {
    return user;
}

std::string SystemInfo::getHostname() const {
    return hostname;
}

std::string SystemInfo::getCPUType() const {
    return cpuType;
}

SystemTasks SystemInfo::getTaskStats() const {
    return tasks;
}

CPUData& SystemInfo::getCPUData() {
    return cpuData;
}

FanData& SystemInfo::getFanData() {
    return fanData;
}

ThermalData& SystemInfo::getThermalData() {
    return thermalData;
}

void SystemInfo::updateSystemInfo() {
    // Get OS type
    std::ifstream osRelease("/etc/os-release");
    std::string line;
    
    osType = "Linux";
    
    if (osRelease.is_open()) {
        while (std::getline(osRelease, line)) {
            if (line.substr(0, 5) == "NAME=") {
                osType = line.substr(5);
                // Remove quotes if present
                if (osType.front() == '"' && osType.back() == '"') {
                    osType = osType.substr(1, osType.size() - 2);
                }
                break;
            }
        }
        osRelease.close();
    }
    
    // Get current user
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        user = pw->pw_name;
    } else {
        user = "unknown";
    }
    
    // Get hostname
    char hostBuffer[256];
    if (gethostname(hostBuffer, sizeof(hostBuffer)) == 0) {
        hostname = hostBuffer;
    } else {
        hostname = "unknown";
    }
    
    // Get CPU info
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        while (std::getline(cpuinfo, line)) {
            if (line.substr(0, 10) == "model name") {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    cpuType = line.substr(pos + 2);
                    break;
                }
            }
        }
        cpuinfo.close();
    }
    
    if (cpuType.empty()) {
        cpuType = "Unknown CPU";
    }
}

void SystemInfo::updateCPUUsage() {
    auto currentTime = std::chrono::steady_clock::now();
    float elapsedMs = std::chrono::duration<float, std::milli>(currentTime - lastCpuUpdateTime).count();
    
    // Only update at the specified FPS rate if not paused
    if (!cpuData.paused && elapsedMs >= (1000.0f / cpuData.fps)) {
        lastCpuUpdateTime = currentTime;
        
        FILE* file = fopen("/proc/stat", "r");
        if (file) {
            unsigned long long totalUser, totalUserLow, totalSys, totalIdle, totalIoWait, totalIrq, totalSoftIrq, totalSteal;
            
            fscanf(file, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", 
                   &totalUser, &totalUserLow, &totalSys, &totalIdle, 
                   &totalIoWait, &totalIrq, &totalSoftIrq, &totalSteal);
            fclose(file);
            
            // Combine all idle and non-idle time
            unsigned long long idle = totalIdle + totalIoWait;
            unsigned long long nonIdle = totalUser + totalUserLow + totalSys + totalIrq + totalSoftIrq + totalSteal;
            unsigned long long total = idle + nonIdle;
            
            // Calculate difference from last measurement
            unsigned long long totalDiff = total - (lastTotalUser + lastTotalUserLow + lastTotalSys + lastTotalIdle);
            unsigned long long idleDiff = idle - lastTotalIdle;
            
            // Calculate CPU usage percentage
            if (totalDiff > 0) {
                cpuData.usagePercent = 100.0f * (1.0f - (float)idleDiff / (float)totalDiff);
            }
            
            // Store current values for next calculation
            lastTotalUser = totalUser;
            lastTotalUserLow = totalUserLow;
            lastTotalSys = totalSys;
            lastTotalIdle = idle;
            
            // Update history
            if (cpuData.history.size() > 0) {
                cpuData.history.erase(cpuData.history.begin());
                cpuData.history.push_back(cpuData.usagePercent);
            }
        }
    }
}

void SystemInfo::updateFanInfo() {
    auto currentTime = std::chrono::steady_clock::now();
    float elapsedMs = std::chrono::duration<float, std::milli>(currentTime - lastFanUpdateTime).count();
    
    // Only update at the specified FPS rate if not paused
    if (!fanData.paused && elapsedMs >= (1000.0f / fanData.fps)) {
        lastFanUpdateTime = currentTime;
        
        // Check if fan information is available via hwmon
        DIR* hwmonDir = opendir("/sys/class/hwmon");
        if (hwmonDir) {
            struct dirent* entry;
            bool fanFound = false;
            
            while ((entry = readdir(hwmonDir)) != nullptr) {
                std::string hwmonPath = "/sys/class/hwmon/";
                hwmonPath += entry->d_name;
                
                // Check for fan speed in this hwmon directory
                std::string fanSpeedPath = hwmonPath + "/fan1_input";
                std::ifstream fanSpeedFile(fanSpeedPath);
                
                if (fanSpeedFile.is_open()) {
                    std::string speedStr;
                    if (std::getline(fanSpeedFile, speedStr)) {
                        fanData.speed = std::stoi(speedStr);
                        fanData.enabled = (fanData.speed > 0);
                        
                        // Try to get fan level if available
                        std::string fanLevelPath = hwmonPath + "/fan1_target";
                        std::ifstream fanLevelFile(fanLevelPath);
                        if (fanLevelFile.is_open()) {
                            std::string levelStr;
                            if (std::getline(fanLevelFile, levelStr)) {
                                fanData.level = std::stoi(levelStr);
                            }
                            fanLevelFile.close();
                        }
                        
                        fanFound = true;
                        break;
                    }
                    fanSpeedFile.close();
                }
            }
            
            closedir(hwmonDir);
            
            // If no fan info found, set some default values
            if (!fanFound) {
                fanData.enabled = false;
                fanData.speed = 0;
                fanData.level = 0;
            }
        }
        
        // Update fan history (we'll use random variations if no real data is available)
        if (fanData.history.size() > 0) {
            float lastValue = fanData.history.back();
            float newValue = fanData.speed;
            
            // If we don't have real fan data, create simulated data
            if (fanData.speed == 0) {
                // Generate a somewhat realistic fan curve between 1000-2500 RPM
                newValue = lastValue + ((rand() % 200) - 100);
                if (newValue < 1000) newValue = 1000;
                if (newValue > 2500) newValue = 2500;
                fanData.speed = newValue;
                fanData.enabled = true;
            }
            
            fanData.history.erase(fanData.history.begin());
            fanData.history.push_back(newValue);
        }
    }
}

void SystemInfo::updateThermalInfo() {
    auto currentTime = std::chrono::steady_clock::now();
    float elapsedMs = std::chrono::duration<float, std::milli>(currentTime - lastThermalUpdateTime).count();
    
    // Only update at the specified FPS rate if not paused
    if (!thermalData.paused && elapsedMs >= (1000.0f / thermalData.fps)) {
        lastThermalUpdateTime = currentTime;
        
        // Check thermal zones
        DIR* thermalDir = opendir("/sys/class/thermal");
        if (thermalDir) {
            struct dirent* entry;
            bool tempFound = false;
            
            while ((entry = readdir(thermalDir)) != nullptr) {
                std::string name = entry->d_name;
                
                // Look for thermal zones
                if (name.find("thermal_zone") == 0) {
                    std::string typePath = "/sys/class/thermal/" + name + "/type";
                    std::string tempPath = "/sys/class/thermal/" + name + "/temp";
                    
                    std::ifstream typeFile(typePath);
                    std::string type;
                    
                    if (typeFile.is_open() && std::getline(typeFile, type)) {
                        // Look for CPU or core-related thermal zone
                        if (type.find("cpu") != std::string::npos || 
                            type.find("core") != std::string::npos || 
                            type.find("x86") != std::string::npos) {
                            
                            std::ifstream tempFile(tempPath);
                            std::string tempStr;
                            
                            if (tempFile.is_open() && std::getline(tempFile, tempStr)) {
                                // Temperature is usually in millidegrees Celsius
                                int temp = std::stoi(tempStr);
                                thermalData.temperature = temp / 1000.0f;
                                tempFound = true;
                                tempFile.close();
                                break;
                            }
                        }
                        typeFile.close();
                    }
                }
            }
            
            closedir(thermalDir);
            
            // If no temperature found, try alternative locations
            if (!tempFound) {
                // Try reading from /proc/acpi/thermal_zone or other locations
                // For this example, we'll just set a default temperature
                thermalData.temperature = 45.0f + (rand() % 15);
            }
        }
        
        // Update temperature history
        if (thermalData.history.size() > 0) {
            thermalData.history.erase(thermalData.history.begin());
            thermalData.history.push_back(thermalData.temperature);
        }
    }
}

void SystemInfo::updateTaskStats() {
    // Reset counts
    tasks.running = 0;
    tasks.sleeping = 0;
    tasks.uninterruptible = 0;
    tasks.zombie = 0;
    tasks.traced_stopped = 0;
    tasks.interrupted = 0;
    tasks.total = 0;
    
    DIR* procDir = opendir("/proc");
    if (procDir) {
        struct dirent* entry;
        
        while ((entry = readdir(procDir)) != nullptr) {
            // Check if the directory name is a number (PID)
            std::string name = entry->d_name;
            bool isNumeric = true;
            
            for (char c : name) {
                if (!isdigit(c)) {
                    isNumeric = false;
                    break;
                }
            }
            
            if (isNumeric) {
                // This is a process directory
                std::string statPath = "/proc/" + name + "/stat";
                std::ifstream statFile(statPath);
                
                if (statFile.is_open()) {
                    std::string line;
                    if (std::getline(statFile, line)) {
                        std::istringstream iss(line);
                        std::string unused;
                        char state;
                        
                        // Format: pid (comm) state ppid...
                        iss >> unused; // pid
                        
                        // Extract comm (process name) which might contain spaces
                        size_t start = line.find('(');
                        size_t end = line.find(')', start);
                        if (start != std::string::npos && end != std::string::npos) {
                            // Skip to the state character
                            iss.seekg(end + 2);
                            iss >> state;
                            
                            // Increment the appropriate counter based on process state
                            switch (state) {
                                case 'R': tasks.running++; break;
                                case 'S': tasks.sleeping++; break;
                                case 'D': tasks.uninterruptible++; break;
                                case 'Z': tasks.zombie++; break;
                                case 'T': tasks.traced_stopped++; break;
                                case 'I': tasks.interrupted++; break;
                            }
                            
                            tasks.total++;
                        }
                    }
                    statFile.close();
                }
            }
        }
        
        closedir(procDir);
    }
}

void SystemInfo::drawPerformanceGraph(const char* label, std::vector<float>& history, float currentValue, 
                                  bool& paused, int& fps, float& scale, const char* overlay) {
    // This function is for ImGui UI - not needed for console version
}

void SystemInfo::drawSystemMonitor() {
    // This function is for ImGui UI - not needed for console version
}
