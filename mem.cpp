#include "header.h"

// MemoryManager Implementation
MemoryManager::MemoryManager(SystemInfo* sys_info) : system_info_ref(sys_info) {
    memset(process_filter, 0, sizeof(process_filter));
}

void MemoryManager::Update() {
    UpdateMemoryInfo();
    UpdateProcesses();
    UpdateDiskInfo();
}

void MemoryManager::UpdateMemoryInfo() {
#ifdef _WIN32
    // Windows memory info
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    system_info_ref->total_memory = memInfo.ullTotalPhys;
    system_info_ref->used_memory = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    system_info_ref->memory_usage = (float)(system_info_ref->used_memory * 100.0 / system_info_ref->total_memory);
    
    // Windows doesn't have traditional swap, but has virtual memory
    system_info_ref->total_swap = memInfo.ullTotalVirtual - memInfo.ullTotalPhys;
    system_info_ref->used_swap = (memInfo.ullTotalVirtual - memInfo.ullAvailVirtual) - system_info_ref->used_memory;
    if (system_info_ref->total_swap > 0) {
        system_info_ref->swap_usage = (float)(system_info_ref->used_swap * 100.0 / system_info_ref->total_swap);
    }
#else
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    std::map<std::string, uint64_t> mem_values;
    
    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;
        std::string unit;
        
        if (iss >> key >> value >> unit) {
            key.pop_back(); // Remove ':'
            mem_values[key] = value * 1024; // Convert to bytes
        }
    }
    
    system_info_ref->total_memory = mem_values["MemTotal"];
    system_info_ref->used_memory = system_info_ref->total_memory - mem_values["MemAvailable"];
    system_info_ref->memory_usage = (float)(system_info_ref->used_memory * 100.0 / system_info_ref->total_memory);
    
    system_info_ref->total_swap = mem_values["SwapTotal"];
    system_info_ref->used_swap = system_info_ref->total_swap - mem_values["SwapFree"];
    if (system_info_ref->total_swap > 0) {
        system_info_ref->swap_usage = (float)(system_info_ref->used_swap * 100.0 / system_info_ref->total_swap);
    }
#endif
}

void MemoryManager::UpdateProcesses() {
#ifdef _WIN32
    // Windows process enumeration
    processes.clear();
    system_info_ref->total_processes = 0;
    system_info_ref->running_processes = 0;
    system_info_ref->sleeping_processes = 0;
    system_info_ref->zombie_processes = 0;
    system_info_ref->stopped_processes = 0;
    
    DWORD process_ids[1024], cbNeeded;
    if (EnumProcesses(process_ids, sizeof(process_ids), &cbNeeded)) {
        DWORD numProcesses = cbNeeded / sizeof(DWORD);
        system_info_ref->total_processes = numProcesses;
        system_info_ref->running_processes = numProcesses; // Simplified for Windows
        
        for (DWORD i = 0; i < numProcesses; i++) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_ids[i]);
            if (hProcess) {
                ProcessInfo proc;
                proc.pid = process_ids[i];
                
                // Get process name
                char processName[MAX_PATH];
                if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) {
                    proc.name = processName;
                } else {
                    proc.name = "Unknown";
                }
                
                // Get memory usage
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    proc.memory_usage = (float)(pmc.WorkingSetSize * 100.0 / system_info_ref->total_memory);
                }
                
                proc.state = "R"; // Simplified for Windows
                proc.cpu_usage = 0.0f; // Would need more complex implementation for real CPU usage
                
                processes.push_back(proc);
                CloseHandle(hProcess);
            }
        }
    }
#else
    processes.clear();
    system_info_ref->total_processes = 0;
    system_info_ref->running_processes = 0;
    system_info_ref->sleeping_processes = 0;
    system_info_ref->zombie_processes = 0;
    system_info_ref->stopped_processes = 0;
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (!isdigit(entry->d_name[0])) continue;
        
        int pid = std::stoi(entry->d_name);
        std::string stat_path = "/proc/" + std::string(entry->d_name) + "/stat";
        std::string status_path = "/proc/" + std::string(entry->d_name) + "/status";
        
        std::ifstream stat_file(stat_path);
        if (!stat_file.is_open()) continue;
        
        std::string line;
        std::getline(stat_file, line);
        
        std::istringstream iss(line);
        std::string pid_str, comm, state;
        long long utime, stime, cutime, cstime, starttime;
        long vsize, rss;
        
        // Parse stat file for basic info
        iss >> pid_str >> comm >> state;
        for (int i = 0; i < 10; ++i) iss >> pid_str; // Skip fields
        iss >> utime >> stime >> cutime >> cstime;
        for (int i = 0; i < 4; ++i) iss >> pid_str; // Skip fields
        iss >> starttime;
        iss >> vsize >> rss;
        
        ProcessInfo proc;
        proc.pid = pid;
        proc.name = comm.substr(1, comm.length() - 2); // Remove parentheses
        proc.state = state;
        
        // Calculate memory usage (RSS in pages, convert to percentage)
        long page_size = sysconf(_SC_PAGESIZE);
        proc.memory_usage = (float)(rss * page_size * 100.0 / system_info_ref->total_memory);
        
        // Simple CPU usage calculation (would need previous values for accurate calculation)
        proc.cpu_usage = 0.0f;
        
        processes.push_back(proc);
        system_info_ref->total_processes++;
        
        if (state == "R") system_info_ref->running_processes++;
        else if (state == "S" || state == "I") system_info_ref->sleeping_processes++;
        else if (state == "Z") system_info_ref->zombie_processes++;
        else if (state == "T") system_info_ref->stopped_processes++;
    }
    
    closedir(proc_dir);
#endif
    
    selected_processes.resize(processes.size(), false);
}

void MemoryManager::UpdateDiskInfo() {
#ifdef _WIN32
    // Windows disk info
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        system_info_ref->total_disk = totalNumberOfBytes.QuadPart;
        system_info_ref->used_disk = totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
        system_info_ref->disk_usage = (float)(system_info_ref->used_disk * 100.0 / system_info_ref->total_disk);
    }
#else
    // Linux disk info using statvfs for root filesystem
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        system_info_ref->total_disk = stat.f_blocks * stat.f_frsize;
        system_info_ref->used_disk = (stat.f_blocks - stat.f_bavail) * stat.f_frsize;
        system_info_ref->disk_usage = (float)(system_info_ref->used_disk * 100.0 / system_info_ref->total_disk);
    }
#endif
}

std::string MemoryManager::FormatBytes(uint64_t bytes) {
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

void MemoryManager::RenderMemoryAndProcesses() {
    // Memory usage section
    ImGui::Text("Physical Memory (RAM):");
    ImGui::ProgressBar(system_info_ref->memory_usage / 100.0f, ImVec2(0, 0), 
                      (FormatBytes(system_info_ref->used_memory) + " / " + FormatBytes(system_info_ref->total_memory)).c_str());
    
    ImGui::Text("Virtual Memory (SWAP):");
    ImGui::ProgressBar(system_info_ref->swap_usage / 100.0f, ImVec2(0, 0), 
                      (FormatBytes(system_info_ref->used_swap) + " / " + FormatBytes(system_info_ref->total_swap)).c_str());
    
    ImGui::Text("Disk Usage:");
    ImGui::ProgressBar(system_info_ref->disk_usage / 100.0f, ImVec2(0, 0), 
                      (FormatBytes(system_info_ref->used_disk) + " / " + FormatBytes(system_info_ref->total_disk)).c_str());
    
    ImGui::Separator();
    
    // Process filter
    ImGui::Text("Filter processes:");
    ImGui::InputText("##filter", process_filter, sizeof(process_filter));
    
    // Process controls
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        UpdateProcesses();
    }
    
    // Process statistics
    ImGui::Text("Total: %d | Running: %d | Sleeping: %d | Zombie: %d | Stopped: %d", 
               system_info_ref->total_processes, system_info_ref->running_processes,
               system_info_ref->sleeping_processes, system_info_ref->zombie_processes,
               system_info_ref->stopped_processes);
    
    ImGui::Separator();
    
    // Process table
    if (ImGui::BeginTable("ProcessTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | 
                         ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("CPU %");
        ImGui::TableSetupColumn("Memory %");
        ImGui::TableHeadersRow();
        
        std::string filter_str = std::string(process_filter);
        std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);
        
        // Sort processes if needed
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty) {
                std::sort(processes.begin(), processes.end(), [&](const ProcessInfo& a, const ProcessInfo& b) {
                    for (int n = 0; n < sort_specs->SpecsCount; n++) {
                        const ImGuiTableColumnSortSpecs* sort_spec = &sort_specs->Specs[n];
                        int delta = 0;
                        switch (sort_spec->ColumnIndex) {
                            case 0: delta = (a.pid < b.pid) ? -1 : (a.pid > b.pid) ? 1 : 0; break;
                            case 1: delta = a.name.compare(b.name); break;
                            case 2: delta = a.state.compare(b.state); break;
                            case 3: delta = (a.cpu_usage < b.cpu_usage) ? -1 : (a.cpu_usage > b.cpu_usage) ? 1 : 0; break;
                            case 4: delta = (a.memory_usage < b.memory_usage) ? -1 : (a.memory_usage > b.memory_usage) ? 1 : 0; break;
                        }
                        if (delta != 0)
                            return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? (delta < 0) : (delta > 0);
                    }
                    return a.pid < b.pid;
                });
                sort_specs->SpecsDirty = false;
            }
        }
        
        for (size_t i = 0; i < processes.size(); ++i) {
            const auto& proc = processes[i];
            
            // Apply filter
            if (!filter_str.empty()) {
                std::string proc_name = proc.name;
                std::transform(proc_name.begin(), proc_name.end(), proc_name.begin(), ::tolower);
                if (proc_name.find(filter_str) == std::string::npos && 
                    std::to_string(proc.pid).find(filter_str) == std::string::npos) {
                    continue;
                }
            }
            
            ImGui::TableNextRow();
            
            // Selectable row
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(std::to_string(proc.pid).c_str(), selected_processes[i], 
                                 ImGuiSelectableFlags_SpanAllColumns)) {
                selected_processes[i] = !selected_processes[i];
            }
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", proc.name.c_str());
            
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", proc.state.c_str());
            
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.1f", proc.cpu_usage);
            
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.2f", proc.memory_usage);
        }
        
        ImGui::EndTable();
    }
    
    // Process actions
    if (ImGui::Button("Kill Selected")) {
        KillSelectedProcesses();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Selection")) {
        std::fill(selected_processes.begin(), selected_processes.end(), false);
    }
}

void MemoryManager::KillSelectedProcesses() {
    for (size_t i = 0; i < processes.size() && i < selected_processes.size(); ++i) {
        if (selected_processes[i]) {
#ifdef _WIN32
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processes[i].pid);
            if (hProcess) {
                TerminateProcess(hProcess, 1);
                CloseHandle(hProcess);
            }
#else
            kill(processes[i].pid, SIGTERM);
#endif
        }
    }
    // Clear selection after killing
    std::fill(selected_processes.begin(), selected_processes.end(), false);
    // Refresh process list
    UpdateProcesses();
}