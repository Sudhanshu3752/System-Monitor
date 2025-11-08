#include "system_info.h"
#include <ncurses.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iostream>

void initializeUI() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(1000);
}

void displaySystemInfo() {
    CPUData cpu = readCPUStats();
    MemoryInfo mem = readMemoryInfo();
    double uptime = getUptime();
    mvprintw(0, 0, "=== System Monitor ===");
    mvprintw(1, 0, "Uptime: %.2f hours", uptime / 3600.0);
    mvprintw(2, 0, "Memory: %ld MB / %ld MB", 
             (mem.totalMem - mem.availableMem) / 1024, 
             mem.totalMem / 1024);
}

void displayProcesses(std::vector<ProcessInfo>& processes, int startRow, int selectedRow) {
    mvprintw(startRow, 0, "%-8s %-10s %-8s %-10s %-8s %s", "PID", "USER", "STATE", "MEM (KB)", "MEM %", "COMMAND");
    int row = startRow + 1;
    for (size_t i = 0; i < processes.size(); ++i) {
        if (row >= LINES - 3) break;
        if (i == selectedRow) attron(A_REVERSE);
        mvprintw(row++, 0, "%-8d %-10s %-8c %-10lu %-8.2f %s",
                 processes[i].pid,
                 processes[i].user.c_str(),
                 processes[i].state,
                 processes[i].memUsage,
                 processes[i].memPercent,
                 processes[i].command.c_str());
        if (i == selectedRow) attroff(A_REVERSE);
    }
}

bool killProcess(int pid) {
    if (kill(pid, SIGTERM) == 0) {
        mvprintw(LINES - 1, 0, "Sent SIGTERM to process %d", pid);
        refresh();
        return true;
    }
    if (kill(pid, SIGKILL) == 0) {
        mvprintw(LINES - 1, 0, "Sent SIGKILL to process %d", pid);
        refresh();
        return true;
    }
    mvprintw(LINES - 1, 0, "Failed to kill process %d: %s", pid, strerror(errno));
    refresh();
    return false;
}

int main() {
    initializeUI();
    int sortMode = 2; // default sort by memory
    int selectedRow = 0;

    while (true) {
        clear();
        displaySystemInfo();

        std::vector<int> pids = getProcessPIDs();
        std::vector<ProcessInfo> processes;

        MemoryInfo mem = readMemoryInfo();
        for (int pid : pids) {
            ProcessInfo proc = readProcessInfo(pid);
            proc.user = getProcessUser(pid);
            proc.memPercent = (mem.totalMem > 0) ? (100.0 * proc.memUsage / mem.totalMem) : 0;
            processes.push_back(proc);
        }
        sortProcesses(processes, sortMode);

        displayProcesses(processes, 4, selectedRow);

        mvprintw(LINES - 3, 0, "Sort: [P]ID [C]PU [M]emory | [Up/Down] Select | [K]ill | [Q]uit");

        int ch = getch();
        switch(ch) {
            case 'p': case 'P': sortMode = 0; break;
            case 'c': case 'C': sortMode = 1; break;
            case 'm': case 'M': sortMode = 2; break;
            case KEY_UP: if (selectedRow > 0) --selectedRow; break;
            case KEY_DOWN: if (selectedRow < (int)processes.size() - 1) ++selectedRow; break;
            case 'k': case 'K':
                if (selectedRow >= 0 && selectedRow < (int)processes.size()) {
                    bool confirm = true; // add confirmation logic if desired
                    if (confirm) killProcess(processes[selectedRow].pid);
                }
                break;
            case 'q': case 'Q':
                endwin();
                return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    endwin();
    return 0;
}
