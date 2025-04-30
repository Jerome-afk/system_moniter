#include "header.h"

NetworkInfo::NetworkInfo() {
    currentTabIndex = 0;
    updateNetworkInterfaces();
}

NetworkInfo::~NetworkInfo() {
}

void NetworkInfo::update() {
    updateNetworkInterfaces();
}

std::vector<NetworkInterface>& NetworkInfo::getInterfaces() {
    return interfaces;
}

void NetworkInfo::updateNetworkInterfaces() {
    // Save previous interfaces data for comparison
    std::map<std::string, NetworkInterface> prevInterfaces;
    for (const auto& iface : interfaces) {
        prevInterfaces[iface.name] = iface;
    }
    
    // Clear current interfaces
    interfaces.clear();
    
    // Get interfaces from /proc/net/dev
    std::ifstream netdev("/proc/net/dev");
    std::string line;
    
    if (netdev.is_open()) {
        // Skip the first two lines (headers)
        std::getline(netdev, line);
        std::getline(netdev, line);
        
        // Process each interface
        while (std::getline(netdev, line)) {
            std::istringstream iss(line);
            NetworkInterface iface;
            
            // Extract interface name (remove the colon)
            iss >> iface.name;
            if (!iface.name.empty() && iface.name.back() == ':') {
                iface.name.pop_back();
            }
            
            // Extract network stats
            iss >> iface.rx_bytes >> iface.rx_packets >> iface.rx_errs >> iface.rx_drop
                >> iface.rx_fifo >> iface.rx_frame >> iface.rx_compressed >> iface.rx_multicast
                >> iface.tx_bytes >> iface.tx_packets >> iface.tx_errs >> iface.tx_drop
                >> iface.tx_fifo >> iface.tx_colls >> iface.tx_carrier >> iface.tx_compressed;
            
            // Get IPv4 address for this interface
            std::string ipPath = "/sys/class/net/" + iface.name + "/address";
            std::ifstream ipFile(ipPath);
            if (ipFile.is_open()) {
                std::getline(ipFile, iface.ipv4);
                ipFile.close();
            }
            
            // Try to get actual IPv4 address from /proc/net/fib_trie or using 'ip' command
            // This is a simplified approach, for actual production code you may need to use netlink or ioctl
            iface.ipv4 = "N/A"; // Default if we can't determine the real IP
            
            // Use /proc/net/if_inet6 for IPv6 (not implemented in this version)
            
            // Calculate usage percentage based on 2GB max (simplification)
            const unsigned long MAX_BANDWIDTH = 2UL * 1024 * 1024 * 1024; // 2GB
            
            // Find previous data for this interface to compute rate
            auto prevIface = prevInterfaces.find(iface.name);
            if (prevIface != prevInterfaces.end()) {
                // For demo, just calculate percentage based on current bytes relative to MAX_BANDWIDTH
                // In a real app, you'd calculate the delta between prev and current readings
                iface.rx_usage_percent = (float)iface.rx_bytes / MAX_BANDWIDTH * 100.0f;
                iface.tx_usage_percent = (float)iface.tx_bytes / MAX_BANDWIDTH * 100.0f;
            } else {
                iface.rx_usage_percent = 0.0f;
                iface.tx_usage_percent = 0.0f;
            }
            
            interfaces.push_back(iface);
        }
        
        netdev.close();
    }
    
    // Get more detailed IP information using 'ip addr' command output
    FILE* pipe = popen("ip -4 addr", "r");
    if (pipe) {
        char buffer[256];
        std::string ipOutput;
        
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            ipOutput += buffer;
        }
        pclose(pipe);
        
        // Process the IP addr output to extract IP addresses for each interface
        for (auto& iface : interfaces) {
            std::string pattern = iface.name + ".*inet\\s+([0-9.]+)";
            std::regex re(pattern);
            std::smatch match;
            
            if (std::regex_search(ipOutput, match, re) && match.size() > 1) {
                iface.ipv4 = match[1].str();
            }
        }
    }
}

std::string NetworkInfo::formatBytes(unsigned long bytes) {
    const float kb = 1024.0f;
    const float mb = kb * 1024.0f;
    const float gb = mb * 1024.0f;
    
    char buffer[32];
    
    if (bytes >= gb) {
        snprintf(buffer, sizeof(buffer), "%.2f GB", bytes / gb);
    } else if (bytes >= mb) {
        snprintf(buffer, sizeof(buffer), "%.2f MB", bytes / mb);
    } else if (bytes >= kb) {
        snprintf(buffer, sizeof(buffer), "%.2f KB", bytes / kb);
    } else {
        snprintf(buffer, sizeof(buffer), "%lu bytes", bytes);
    }
    
    return std::string(buffer);
}

void NetworkInfo::drawNetworkMonitor() {
    // This function is for ImGui UI - not needed for console version
}
