#include "system_info.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <algorithm>

CPUData readCPUStats() {
    CPUData data = {0};
    FILE* file = fopen("/proc/stat", "r");
    if (file) {
        fscanf(file, "cpu %llu %llu %llu %llu", 
               &data.user, &data.nice, &data.system, &data.idle);
        fclose(file);

        data.active = data.user + data.nice + data.system;
        data.total = data.active + data.idle;
    }
    return data;
}

MemoryInfo readMemoryInfo() {
    MemoryInfo mem = {0};
    FILE* file = fopen("/proc/meminfo", "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "MemTotal: %ld kB", &mem.totalMem) == 1) continue;
            if (sscanf(line, "MemFree: %ld kB", &mem.freeMem) == 1) continue;
            if (sscanf(line, "MemAvailable: %ld kB", &mem.availableMem) == 1) break;
        }
        fclose(file);
    }
    return mem;
}

double getUptime() {
    double uptime = 0.0;
    FILE* file = fopen("/proc/uptime", "r");
    if (file) {
        fscanf(file, "%lf", &uptime);
        fclose(file);
    }
    return uptime;
}

std::vector<int> getProcessPIDs() {
    std::vector<int> pids;
    DIR* dir = opendir("/proc");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                int pid = atoi(entry->d_name);
                if (pid > 0) pids.push_back(pid);
            }
        }
        closedir(dir);
    }
    return pids;
}

ProcessInfo readProcessInfo(int pid) {
    ProcessInfo proc;
    proc.pid = pid;
    proc.cpuPercent = 0.0;
    proc.memPercent = 0.0;
    proc.memUsage = 0;
    proc.state = '?';
    // /proc/[pid]/stat
    char statPath[256];
    snprintf(statPath, sizeof(statPath), "/proc/%d/stat", pid);

    FILE* file = fopen(statPath, "r");
    if (file) {
        unsigned long utime = 0, stime = 0;
        long rss = 0;
        char comm[256] = {0};
        fscanf(file, "%d (%[^)]) %c", &proc.pid, comm, &proc.state);
        for (int i = 0; i < 11; ++i) fscanf(file, "%*s");
        fscanf(file, "%lu %lu", &utime, &stime);
        for (int i = 0; i < 7; ++i) fscanf(file, "%*s");
        fscanf(file, "%ld", &rss);
        fclose(file);

        proc.memUsage = rss * sysconf(_SC_PAGESIZE) / 1024; // KB
        proc.command = comm;
    }

    // /proc/[pid]/cmdline
    char cmdPath[256];
    snprintf(cmdPath, sizeof(cmdPath), "/proc/%d/cmdline", pid);
    file = fopen(cmdPath, "r");
    if (file) {
        char cmd[256] = {0};
        fread(cmd, 1, sizeof(cmd) - 1, file);
        if (cmd[0] != 0) proc.command = cmd;
        fclose(file);
    }
    return proc;
}

std::string getProcessUser(int pid) {
    char statusPath[256];
    snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", pid);

    FILE* file = fopen(statusPath, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            int uid;
            if (sscanf(line, "Uid:\t%d", &uid) == 1) {
                fclose(file);
                struct passwd* pw = getpwuid(uid);
                return pw ? pw->pw_name : "unknown";
            }
        }
        fclose(file);
    }
    return "unknown";
}

void sortProcesses(std::vector<ProcessInfo>& processes, int mode) {
    if (mode == 0) { // PID
        std::sort(processes.begin(), processes.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.pid < b.pid;
        });
    } else if (mode == 1) { // CPU (not implemented fully here as true per-process CPU requires sampling)
        std::sort(processes.begin(), processes.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.cpuPercent > b.cpuPercent;
        });
    } else if (mode == 2) { // Memory
        std::sort(processes.begin(), processes.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.memUsage > b.memUsage;
        });
    }
}
