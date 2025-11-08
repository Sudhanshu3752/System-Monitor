#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <string>
#include <vector>

struct CPUData {
    unsigned long long user, nice, system, idle;
    unsigned long long total, active;
};

struct MemoryInfo {
    long totalMem;
    long freeMem;
    long availableMem;
};

struct ProcessInfo {
    int pid;
    std::string user;
    std::string command;
    float cpuPercent;
    float memPercent;
    unsigned long memUsage; // in KB
    char state;
};

CPUData readCPUStats();
MemoryInfo readMemoryInfo();
double getUptime();
std::vector<int> getProcessPIDs();
ProcessInfo readProcessInfo(int pid);
std::string getProcessUser(int pid);
void sortProcesses(std::vector<ProcessInfo>&, int);

#endif
