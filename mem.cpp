#include "header.h"

MemoryInfo::MemoryInfo() {
    // Initialize filter text
    filterText = "";
    
    // Initial update
    updateMemoryInfo();
    updateSwapInfo();
    updateDiskInfo();
    updateProcesses();
}

MemoryInfo::~MemoryInfo() {
}

void MemoryInfo::update() {
    updateMemoryInfo();
    updateSwapInfo();
    updateDiskInfo();
    updateProcesses();
}

std::vector<ProcessInfo>& MemoryInfo::getProcesses() {
    return processes;
}

MemoryData& MemoryInfo::getRAMData() {
    return ram;
}

SwapData& MemoryInfo::getSwapData() {
    return swap;
}

DiskData& MemoryInfo::getDiskData() {
    return disk;
}

void MemoryInfo::updateMemoryInfo() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    
    unsigned long memTotal = 0;
    unsigned long memFree = 0;
    unsigned long memAvailable = 0;
    unsigned long buffers = 0;
    unsigned long cached = 0;
    
    if (meminfo.is_open()) {
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> memTotal;
            } else if (line.find("MemFree:") == 0) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> memFree;
            } else if (line.find("MemAvailable:") == 0) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> memAvailable;
            } else if (line.find("Buffers:") == 0) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> buffers;
            } else if (line.find("Cached:") == 0 && line.find("SwapCached:") != 0) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> cached;
            }
        }
        meminfo.close();
    }
    
    // Convert from KB to bytes
    ram.total = memTotal * 1024;
    ram.free = memFree * 1024;
    
    // Calculate used memory (accounting for buffers and cache)
    unsigned long actuallyUsed = memTotal - memFree - buffers - cached;
    ram.used = actuallyUsed * 1024;
    
    // Calculate percentage
    if (ram.total > 0) {
        ram.usedPercent = (float)ram.used / (float)ram.total * 100.0f;
    } else {
        ram.usedPercent = 0.0f;
    }
}

void MemoryInfo::updateSwapInfo() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    
    unsigned long swapTotal = 0;
    unsigned long swapFree = 0;
    
    if (meminfo.is_open()) {
        while (std::getline(meminfo, line)) {
            if (line.find("SwapTotal:") == 0) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> swapTotal;
            } else if (line.find("SwapFree:") == 0) {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> swapFree;
            }
        }
        meminfo.close();
    }
    
    // Convert from KB to bytes
    swap.total = swapTotal * 1024;
    swap.free = swapFree * 1024;
    swap.used = swap.total - swap.free;
    
    // Calculate percentage
    if (swap.total > 0) {
        swap.usedPercent = (float)swap.used / (float)swap.total * 100.0f;
    } else {
        swap.usedPercent = 0.0f;
    }
}

void MemoryInfo::updateDiskInfo() {
    struct statvfs stat;
    
    // Get root filesystem stats
    if (statvfs("/", &stat) == 0) {
        // Calculate total, free, and used space
        disk.total = (unsigned long)stat.f_blocks * stat.f_frsize;
        disk.free = (unsigned long)stat.f_bfree * stat.f_frsize;
        disk.used = disk.total - disk.free;
        
        // Calculate percentage
        if (disk.total > 0) {
            disk.usedPercent = (float)disk.used / (float)disk.total * 100.0f;
        } else {
            disk.usedPercent = 0.0f;
        }
    }
}

float MemoryInfo::calculateProcessCpuUsage(int pid, unsigned long long processCpuTime) {
    auto currentTime = std::chrono::steady_clock::now();
    
    // Get the last time we checked this process
    auto lastUpdateIter = lastProcessUpdateTime.find(pid);
    auto lastCpuTimeIter = lastProcessCpuTime.find(pid);
    
    float cpuUsage = 0.0f;
    
    if (lastUpdateIter != lastProcessUpdateTime.end() && lastCpuTimeIter != lastProcessCpuTime.end()) {
        // Calculate time difference
        float elapsedSecs = std::chrono::duration<float>(currentTime - lastUpdateIter->second).count();
        
        // Calculate CPU time difference
        unsigned long long cpuTimeDiff = processCpuTime - lastCpuTimeIter->second;
        
        // Calculate CPU usage percentage (considering all cores)
        int numCPUs = sysconf(_SC_NPROCESSORS_ONLN);
        if (elapsedSecs > 0 && numCPUs > 0) {
            cpuUsage = (cpuTimeDiff / elapsedSecs) * 100.0f / numCPUs;
        }
    }
    
    // Update the last checked time and CPU time
    lastProcessUpdateTime[pid] = currentTime;
    lastProcessCpuTime[pid] = processCpuTime;
    
    return cpuUsage;
}

void MemoryInfo::updateProcesses() {
    processes.clear();
    
    DIR* procDir = opendir("/proc");
    if (procDir) {
        struct dirent* entry;
        long pageSizeKB = sysconf(_SC_PAGESIZE) / 1024; // Page size in KB
        
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
                int pid = std::stoi(name);
                ProcessInfo process;
                process.pid = pid;
                process.selected = false;
                
                // Get process name from stat file
                std::string statPath = "/proc/" + name + "/stat";
                std::ifstream statFile(statPath);
                
                if (statFile.is_open()) {
                    std::string line;
                    if (std::getline(statFile, line)) {
                        std::istringstream iss(line);
                        std::string unused;
                        char state;
                        
                        // Extract the process name from between parentheses
                        size_t start = line.find('(');
                        size_t end = line.rfind(')');
                        
                        if (start != std::string::npos && end != std::string::npos && end > start) {
                            process.name = line.substr(start + 1, end - start - 1);
                            
                            // Skip to the state character
                            iss.seekg(end + 2);
                            iss >> state;
                            
                            // Map state character to descriptive string
                            switch (state) {
                                case 'R': process.state = "Running"; break;
                                case 'S': process.state = "Sleeping"; break;
                                case 'D': process.state = "Disk Sleep"; break;
                                case 'Z': process.state = "Zombie"; break;
                                case 'T': process.state = "Stopped"; break;
                                case 't': process.state = "Tracing"; break;
                                case 'X': process.state = "Dead"; break;
                                case 'I': process.state = "Idle"; break;
                                default: process.state = "Unknown";
                            }
                            
                            // Skip to the CPU time fields
                            for (int i = 0; i < 11; i++) iss >> unused;
                            
                            // Read utime, stime, cutime, cstime
                            unsigned long utime, stime, cutime, cstime;
                            iss >> utime >> stime >> cutime >> cstime;
                            
                            // Total CPU time for this process in clock ticks
                            unsigned long long totalCpuTime = utime + stime + cutime + cstime;
                            
                            // Calculate CPU usage percentage
                            process.cpuUsage = calculateProcessCpuUsage(pid, totalCpuTime);
                        }
                    }
                    statFile.close();
                }
                
                // Get memory usage from status file
                std::string statusPath = "/proc/" + name + "/status";
                std::ifstream statusFile(statusPath);
                
                if (statusFile.is_open()) {
                    std::string line;
                    unsigned long vmRSS = 0;
                    
                    while (std::getline(statusFile, line)) {
                        if (line.find("VmRSS:") == 0) {
                            std::istringstream iss(line);
                            std::string dummy;
                            iss >> dummy >> vmRSS;
                            break;
                        }
                    }
                    statusFile.close();
                    
                    // Calculate memory usage percentage based on total system RAM
                    if (ram.total > 0) {
                        process.memoryUsage = (float)(vmRSS * 1024) / (float)ram.total * 100.0f;
                    } else {
                        process.memoryUsage = 0.0f;
                    }
                }
                
                processes.push_back(process);
            }
        }
        
        closedir(procDir);
    }
    
    // Sort processes by CPU usage (descending)
    std::sort(processes.begin(), processes.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.cpuUsage > b.cpuUsage;
              });
}

void MemoryInfo::drawMemoryMonitor() {
    // This function is for ImGui UI - not needed for console version
}
