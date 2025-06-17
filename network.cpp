#include "header.h"

// NetworkManager Implementation
NetworkManager::NetworkManager() {
    // Initialize previous values for rate calculation
    previous_update_time = std::chrono::steady_clock::now();
}

void NetworkManager::Update() {
    UpdateNetworkInterfaces();
    CalculateNetworkRates();
}

void NetworkManager::UpdateNetworkInterfaces() {
#ifdef _WIN32
    UpdateNetworkInterfacesWindows();
#else
    UpdateNetworkInterfacesLinux();
#endif
}

#ifdef _WIN32
void NetworkManager::UpdateNetworkInterfacesWindows() {
    network_interfaces.clear();
    
    // Get adapter info
    PIP_ADAPTER_INFO pAdapterInfo;
    DWORD dwRetVal = 0;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    
    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) return;
    
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) return;
    }
    
    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            NetworkInterface iface;
            iface.name = pAdapter->AdapterName;
            iface.description = pAdapter->Description;
            iface.ipv4 = pAdapter->IpAddressList.IpAddress.String;
            iface.type = pAdapter->Type;
            
            // Get interface statistics
            MIB_IFROW ifRow;
            ifRow.dwIndex = pAdapter->Index;
            if (GetIfEntry(&ifRow) == NO_ERROR) {
                iface.rx_bytes = ifRow.dwInOctets;
                iface.tx_bytes = ifRow.dwOutOctets;
                iface.rx_packets = ifRow.dwInUcastPkts + ifRow.dwInNUcastPkts;
                iface.tx_packets = ifRow.dwOutUcastPkts + ifRow.dwOutNUcastPkts;
                iface.rx_errs = ifRow.dwInErrors;
                iface.tx_errs = ifRow.dwOutErrors;
                iface.rx_drop = ifRow.dwInDiscards;
                iface.tx_drop = ifRow.dwOutDiscards;
                iface.operational_status = (ifRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL);
            }
            
            network_interfaces.push_back(iface);
            pAdapter = pAdapter->Next;
        }
    }
    
    if (pAdapterInfo)
        free(pAdapterInfo);
}
#else
void NetworkManager::UpdateNetworkInterfacesLinux() {
    network_interfaces.clear();
    
    // Read from /proc/net/dev
    std::ifstream net_dev("/proc/net/dev");
    std::string line;
    
    // Skip header lines
    std::getline(net_dev, line);
    std::getline(net_dev, line);
    
    while (std::getline(net_dev, line)) {
        std::istringstream iss(line);
        std::string iface_name;
        
        if (std::getline(iss, iface_name, ':')) {
            // Trim whitespace
            iface_name.erase(0, iface_name.find_first_not_of(" \t"));
            iface_name.erase(iface_name.find_last_not_of(" \t") + 1);
            
            NetworkInterface iface;
            iface.name = iface_name;
            
            // Parse network statistics
            iss >> iface.rx_bytes >> iface.rx_packets >> iface.rx_errs >> iface.rx_drop
                >> iface.rx_fifo >> iface.rx_frame >> iface.rx_compressed >> iface.rx_multicast
                >> iface.tx_bytes >> iface.tx_packets >> iface.tx_errs >> iface.tx_drop
                >> iface.tx_fifo >> iface.tx_colls >> iface.tx_carrier >> iface.tx_compressed;
            
            // Get IP address and additional info
            GetInterfaceDetails(iface);
            
            network_interfaces.push_back(iface);
        }
    }
}

void NetworkManager::GetInterfaceDetails(NetworkInterface& iface) {
    // Get IP address using getifaddrs
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && std::string(ifa->ifa_name) == iface.name) {
                if (ifa->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                    iface.ipv4 = inet_ntoa(addr_in->sin_addr);
                } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                    struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)ifa->ifa_addr;
                    char ipv6_str[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
                    iface.ipv6 = ipv6_str;
                }
                
                // Check if interface is up
                iface.operational_status = (ifa->ifa_flags & IFF_UP) && (ifa->ifa_flags & IFF_RUNNING);
            }
        }
        freeifaddrs(ifaddr);
    }
    
    // Get interface type and additional details
    std::string sys_path = "/sys/class/net/" + iface.name + "/type";
    std::ifstream type_file(sys_path);
    if (type_file.is_open()) {
        type_file >> iface.type;
    }
    
    // Get interface speed
    sys_path = "/sys/class/net/" + iface.name + "/speed";
    std::ifstream speed_file(sys_path);
    if (speed_file.is_open()) {
        speed_file >> iface.speed_mbps;
    }
    
    // Get MAC address
    sys_path = "/sys/class/net/" + iface.name + "/address";
    std::ifstream mac_file(sys_path);
    if (mac_file.is_open()) {
        std::getline(mac_file, iface.mac_address);
    }
}
#endif

void NetworkManager::CalculateNetworkRates() {
    auto current_time = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(current_time - previous_update_time).count();
    
    if (time_diff > 0 && !previous_interfaces.empty()) {
        for (auto& current_iface : network_interfaces) {
            // Find matching previous interface
            auto prev_it = std::find_if(previous_interfaces.begin(), previous_interfaces.end(),
                [&current_iface](const NetworkInterface& prev) {
                    return prev.name == current_iface.name;
                });
            
            if (prev_it != previous_interfaces.end()) {
                // Calculate rates (bytes per second)
                current_iface.rx_rate = (current_iface.rx_bytes - prev_it->rx_bytes) / time_diff;
                current_iface.tx_rate = (current_iface.tx_bytes - prev_it->tx_bytes) / time_diff;
                
                // Calculate packet rates
                current_iface.rx_packet_rate = (current_iface.rx_packets - prev_it->rx_packets) / time_diff;
                current_iface.tx_packet_rate = (current_iface.tx_packets - prev_it->tx_packets) / time_diff;
            }
        }
    }
    
    previous_interfaces = network_interfaces;
    previous_update_time = current_time;
}

std::string NetworkManager::FormatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double value = (double)bytes;
    
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        unit++;
    }
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unit]);
    return std::string(buffer);
}

std::string NetworkManager::FormatRate(uint64_t bytes_per_sec) {
    return FormatBytes(bytes_per_sec) + "/s";
}

void NetworkManager::RenderNetwork() {}

void NetworkManager::RenderNetworkInfo() {
    // Network interface summary
    ImGui::Text("Network Interfaces: %zu", network_interfaces.size());
    ImGui::Separator();
    
    // Interface overview
    for (const auto& iface : network_interfaces) {
        ImGui::Text("Interface: %s", iface.name.c_str());
        ImGui::SameLine();
        if (iface.operational_status) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[UP]");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[DOWN]");
        }
        
        ImGui::Text("  IPv4: %s", iface.ipv4.empty() ? "N/A" : iface.ipv4.c_str());
        if (!iface.ipv6.empty()) {
            ImGui::Text("  IPv6: %s", iface.ipv6.c_str());
        }
        if (!iface.mac_address.empty()) {
            ImGui::Text("  MAC: %s", iface.mac_address.c_str());
        }
        if (iface.speed_mbps > 0) {
            ImGui::Text("  Speed: %d Mbps", iface.speed_mbps);
        }
        
        // Real-time rates
        ImGui::Text("  RX Rate: %s", FormatRate(iface.rx_rate).c_str());
        ImGui::Text("  TX Rate: %s", FormatRate(iface.tx_rate).c_str());
        
        ImGui::Separator();
    }
    
    // Network tables
    if (ImGui::BeginTabBar("NetworkTabs")) {
        if (ImGui::BeginTabItem("RX (Receive)")) {
            RenderNetworkTable(true);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("TX (Transmit)")) {
            RenderNetworkTable(false);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Statistics")) {
            RenderNetworkStatistics();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    // Network usage visualization
    ImGui::Separator();
    ImGui::Text("Network Usage Visualization:");
    
    for (const auto& iface : network_interfaces) {
        if (iface.name == "lo") continue; // Skip loopback interface
        
        float rx_gb = (float)iface.rx_bytes / (1024.0f * 1024.0f * 1024.0f);
        float tx_gb = (float)iface.tx_bytes / (1024.0f * 1024.0f * 1024.0f);
        float max_gb = 10.0f; // 10GB scale
        
        ImGui::Text("%s:", iface.name.c_str());
        ImGui::Text("  RX: %s (%s)", FormatBytes(iface.rx_bytes).c_str(), FormatRate(iface.rx_rate).c_str());
        ImGui::ProgressBar(std::min(rx_gb / max_gb, 1.0f), ImVec2(0, 0));
        
        ImGui::Text("  TX: %s (%s)", FormatBytes(iface.tx_bytes).c_str(), FormatRate(iface.tx_rate).c_str());
        ImGui::ProgressBar(std::min(tx_gb / max_gb, 1.0f), ImVec2(0, 0));
        
        ImGui::Separator();
    }
}

void NetworkManager::RenderNetworkTable(bool is_rx) {
    if (ImGui::BeginTable(is_rx ? "RXTable" : "TXTable", 8, 
                         ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | 
                         ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        
        ImGui::TableSetupColumn("Interface");
        ImGui::TableSetupColumn("Bytes");
        ImGui::TableSetupColumn("Packets");
        ImGui::TableSetupColumn("Errors");
        ImGui::TableSetupColumn("Drops");
        if (is_rx) {
            ImGui::TableSetupColumn("FIFO");
            ImGui::TableSetupColumn("Frame");
            ImGui::TableSetupColumn("Compressed");
        } else {
            ImGui::TableSetupColumn("FIFO");
            ImGui::TableSetupColumn("Colls");
            ImGui::TableSetupColumn("Carrier");
        }
        ImGui::TableHeadersRow();
        
        for (const auto& iface : network_interfaces) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", iface.name.c_str());
            
            if (is_rx) {
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", FormatBytes(iface.rx_bytes).c_str());
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%llu", (unsigned long long)iface.rx_packets);
                
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%llu", (unsigned long long)iface.rx_errs);
                
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%llu", (unsigned long long)iface.rx_drop);
                
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%llu", (unsigned long long)iface.rx_fifo);
                
                ImGui::TableSetColumnIndex(6);
                ImGui::Text("%llu", (unsigned long long)iface.rx_frame);
                
                ImGui::TableSetColumnIndex(7);
                ImGui::Text("%llu", (unsigned long long)iface.rx_compressed);
            } else {
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", FormatBytes(iface.tx_bytes).c_str());
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%llu", (unsigned long long)iface.tx_packets);
                
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%llu", (unsigned long long)iface.tx_errs);
                
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%llu", (unsigned long long)iface.tx_drop);
                
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%llu", (unsigned long long)iface.tx_fifo);
                
                ImGui::TableSetColumnIndex(6);
                ImGui::Text("%llu", (unsigned long long)iface.tx_colls);
                
                ImGui::TableSetColumnIndex(7);
                ImGui::Text("%llu", (unsigned long long)iface.tx_carrier);
            }
        }
        
        ImGui::EndTable();
    }
}

void NetworkManager::RenderNetworkStatistics() {
    if (ImGui::BeginTable("NetworkStats", 6, 
                         ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | 
                         ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        
        ImGui::TableSetupColumn("Interface");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Total RX");
        ImGui::TableSetupColumn("Total TX");
        ImGui::TableSetupColumn("RX Rate");
        ImGui::TableSetupColumn("TX Rate");
        ImGui::TableHeadersRow();
        
        for (const auto& iface : network_interfaces) {
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", iface.name.c_str());
            
            ImGui::TableSetColumnIndex(1);
            if (iface.operational_status) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "UP");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DOWN");
            }
            
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", FormatBytes(iface.rx_bytes).c_str());
            
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", FormatBytes(iface.tx_bytes).c_str());
            
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", FormatRate(iface.rx_rate).c_str());
            
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%s", FormatRate(iface.tx_rate).c_str());
        }
        
        ImGui::EndTable();
    }
    
    // Additional statistics
    ImGui::Separator();
    ImGui::Text("Network Summary:");
    
    uint64_t total_rx = 0, total_tx = 0;
    uint64_t total_rx_rate = 0, total_tx_rate = 0;
    int active_interfaces = 0;
    
    for (const auto& iface : network_interfaces) {
        if (iface.name != "lo") { // Skip loopback
            total_rx += iface.rx_bytes;
            total_tx += iface.tx_bytes;
            total_rx_rate += iface.rx_rate;
            total_tx_rate += iface.tx_rate;
            if (iface.operational_status) {
                active_interfaces++;
            }
        }
    }
    
    ImGui::Text("Active Interfaces: %d", active_interfaces);
    ImGui::Text("Total Data Received: %s", FormatBytes(total_rx).c_str());
    ImGui::Text("Total Data Transmitted: %s", FormatBytes(total_tx).c_str());
    ImGui::Text("Current RX Rate: %s", FormatRate(total_rx_rate).c_str());
    ImGui::Text("Current TX Rate: %s", FormatRate(total_tx_rate).c_str());
    
    // Calculate total traffic
    uint64_t total_traffic = total_rx + total_tx;
    ImGui::Text("Total Network Traffic: %s", FormatBytes(total_traffic).c_str());
    
    // Show packet statistics
    ImGui::Separator();
    ImGui::Text("Packet Statistics:");
    
    uint64_t total_rx_packets = 0, total_tx_packets = 0;
    uint64_t total_rx_errors = 0, total_tx_errors = 0;
    uint64_t total_rx_drops = 0, total_tx_drops = 0;
    
    for (const auto& iface : network_interfaces) {
        if (iface.name != "lo") { // Skip loopback
            total_rx_packets += iface.rx_packets;
            total_tx_packets += iface.tx_packets;
            total_rx_errors += iface.rx_errs;
            total_tx_errors += iface.tx_errs;
            total_rx_drops += iface.rx_drop;
            total_tx_drops += iface.tx_drop;
        }
    }
    
    ImGui::Text("Total Packets Received: %llu", (unsigned long long)total_rx_packets);
    ImGui::Text("Total Packets Transmitted: %llu", (unsigned long long)total_tx_packets);
    ImGui::Text("Total RX Errors: %llu", (unsigned long long)total_rx_errors);
    ImGui::Text("Total TX Errors: %llu", (unsigned long long)total_tx_errors);
    ImGui::Text("Total RX Drops: %llu", (unsigned long long)total_rx_drops);
    ImGui::Text("Total TX Drops: %llu", (unsigned long long)total_tx_drops);
    
    // Calculate error rates
    if (total_rx_packets > 0) {
        float rx_error_rate = (float)(total_rx_errors * 100.0 / total_rx_packets);
        ImGui::Text("RX Error Rate: %.2f%%", rx_error_rate);
    }
    
    if (total_tx_packets > 0) {
        float tx_error_rate = (float)(total_tx_errors * 100.0 / total_tx_packets);
        ImGui::Text("TX Error Rate: %.2f%%", tx_error_rate);
    }
}