#include "header.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

// Simple console-based system monitor application
int main() {
    // Setup system classes
    SystemInfo systemInfo;
    MemoryInfo memoryInfo;
    NetworkInfo networkInfo;
    
    std::cout << "===== System Monitor Application =====" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl << std::endl;
    
    // Main loop
    while (true) {
        // Clear screen (works on most terminals)
        std::cout << "\033[2J\033[1;1H";
        
        // Update data
        systemInfo.update();
        memoryInfo.update();
        networkInfo.update();
        
        // Display system information
        std::cout << "=== System Information ===" << std::endl;
        std::cout << "OS: " << systemInfo.getOSType() << std::endl;
        std::cout << "User: " << systemInfo.getUser() << std::endl;
        std::cout << "Hostname: " << systemInfo.getHostname() << std::endl;
        std::cout << "CPU: " << systemInfo.getCPUType() << std::endl;
        
        // Get task statistics
        SystemTasks tasks = systemInfo.getTaskStats();
        std::cout << "Total Tasks: " << tasks.total 
                 << " (Running: " << tasks.running 
                 << ", Sleeping: " << tasks.sleeping 
                 << ", Uninterruptible: " << tasks.uninterruptible 
                 << ", Zombie: " << tasks.zombie 
                 << ", Traced/Stopped: " << tasks.traced_stopped 
                 << ", Interrupted: " << tasks.interrupted << ")" << std::endl;
        
        // Display CPU, Fan and Thermal information
        std::cout << std::endl << "=== Performance Information ===" << std::endl;
        std::cout << "CPU Usage: " << std::fixed << std::setprecision(1) 
                 << systemInfo.getCPUData().usagePercent << "%" << std::endl;
                 
        FanData& fan = systemInfo.getFanData();
        std::cout << "Fan: " << (fan.enabled ? "Enabled" : "Disabled") 
                 << ", Speed: " << fan.speed << " RPM, Level: " << fan.level << std::endl;
                 
        std::cout << "Temperature: " << std::fixed << std::setprecision(1) 
                 << systemInfo.getThermalData().temperature << "°C" << std::endl;
        
        // Display memory information
        std::cout << std::endl << "=== Memory Information ===" << std::endl;
        
        // RAM
        MemoryData& ram = memoryInfo.getRAMData();
        float ramUsedGB = ram.used / (1024.0f * 1024.0f * 1024.0f);
        float ramTotalGB = ram.total / (1024.0f * 1024.0f * 1024.0f);
        std::cout << "RAM: " << std::fixed << std::setprecision(2) 
                 << ramUsedGB << " GB / " << ramTotalGB << " GB (" 
                 << std::setprecision(1) << ram.usedPercent << "%)" << std::endl;
        
        // Swap
        SwapData& swap = memoryInfo.getSwapData();
        float swapUsedGB = swap.used / (1024.0f * 1024.0f * 1024.0f);
        float swapTotalGB = swap.total / (1024.0f * 1024.0f * 1024.0f);
        std::cout << "Swap: " << std::fixed << std::setprecision(2) 
                 << swapUsedGB << " GB / " << swapTotalGB << " GB (" 
                 << std::setprecision(1) << swap.usedPercent << "%)" << std::endl;
        
        // Disk
        DiskData& disk = memoryInfo.getDiskData();
        float diskUsedGB = disk.used / (1024.0f * 1024.0f * 1024.0f);
        float diskTotalGB = disk.total / (1024.0f * 1024.0f * 1024.0f);
        std::cout << "Disk: " << std::fixed << std::setprecision(2) 
                 << diskUsedGB << " GB / " << diskTotalGB << " GB (" 
                 << std::setprecision(1) << disk.usedPercent << "%)" << std::endl;
        
        // Display process information (top 5 CPU consumers)
        std::cout << std::endl << "=== Top Processes (CPU) ===" << std::endl;
        std::cout << std::left << std::setw(8) << "PID" 
                 << std::setw(20) << "Name" 
                 << std::setw(12) << "State" 
                 << std::setw(10) << "CPU%" 
                 << std::setw(10) << "Memory%" << std::endl;
        
        int count = 0;
        for (const auto& process : memoryInfo.getProcesses()) {
            std::cout << std::left << std::setw(8) << process.pid 
                     << std::setw(20) << (process.name.length() > 19 ? process.name.substr(0, 16) + "..." : process.name)
                     << std::setw(12) << process.state 
                     << std::setw(10) << std::fixed << std::setprecision(1) << process.cpuUsage
                     << std::setw(10) << std::fixed << std::setprecision(1) << process.memoryUsage << std::endl;
            
            if (++count >= 5) break;
        }
        
        // Display network information
        std::cout << std::endl << "=== Network Interfaces ===" << std::endl;
        for (const auto& iface : networkInfo.getInterfaces()) {
            std::cout << iface.name << ": " << iface.ipv4 << std::endl;
            std::cout << "  RX: " << networkInfo.formatBytes(iface.rx_bytes) 
                     << " (" << std::fixed << std::setprecision(1) << iface.rx_usage_percent << "%)" << std::endl;
            std::cout << "  TX: " << networkInfo.formatBytes(iface.tx_bytes)
                     << " (" << std::fixed << std::setprecision(1) << iface.tx_usage_percent << "%)" << std::endl;
        }
        
        // Sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}