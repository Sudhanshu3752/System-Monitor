#include "system_info.h"
#include <ncurses.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>

void initColors() {
    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);     // Header and borders
    init_pair(2, COLOR_BLACK, COLOR_CYAN);     // Highlight selected row
    init_pair(3, COLOR_WHITE, -1);              // Normal text
    init_pair(4, COLOR_GREEN, -1);              // System info or positive info
    init_pair(5, COLOR_RED, -1);                // Error or warnings
}

void drawBox(WINDOW* win) {
    wbkgd(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wrefresh(win);
}

void displaySystemInfo(WINDOW* win) {
    werase(win);
    wattron(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, 1, 2, "System Information");
    wattroff(win, COLOR_PAIR(4) | A_BOLD);

    CPUData cpu = readCPUStats();
    MemoryInfo mem = readMemoryInfo();
    double uptime = getUptime();

    mvwprintw(win, 3, 2, "Uptime: %.2f hours", uptime / 3600.0);
    mvwprintw(win, 4, 2, "Memory: %ld MB / %ld MB", (mem.totalMem - mem.availableMem) / 1024, mem.totalMem / 1024);

    drawBox(win);
}

void displayProcessList(WINDOW* win, const std::vector<ProcessInfo>& processes, int selectedRow) {
    werase(win);

    wattron(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(1));

    wattron(win, A_BOLD | COLOR_PAIR(4));
    mvwprintw(win, 1, 2, "PID");
    mvwprintw(win, 1, 10, "USER");
    mvwprintw(win, 1, 22, "STATE");
    mvwprintw(win, 1, 30, "MEM(KB)");
    mvwprintw(win, 1, 40, "MEM%%");
    mvwprintw(win, 1, 46, "COMMAND");
    wattroff(win, A_BOLD | COLOR_PAIR(4));

    int maxRows, maxCols;
    getmaxyx(win, maxRows, maxCols);

    int displayCount = maxRows - 3;
    for (int i = 0; i < displayCount && i < (int)processes.size(); ++i) {
        int y = i + 2;
        if (i == selectedRow) {
            wattron(win, COLOR_PAIR(2));
            mvwprintw(win, y, 2, "%-6d %-10s %-6c %-8lu %6.2f %s",
                      processes[i].pid, processes[i].user.c_str(), processes[i].state,
                      processes[i].memUsage, processes[i].memPercent, processes[i].command.c_str());
            wattroff(win, COLOR_PAIR(2));
        } else {
            wattron(win, COLOR_PAIR(3));
            mvwprintw(win, y, 2, "%-6d %-10s %-6c %-8lu %6.2f %s",
                      processes[i].pid, processes[i].user.c_str(), processes[i].state,
                      processes[i].memUsage, processes[i].memPercent, processes[i].command.c_str());
            wattroff(win, COLOR_PAIR(3));
        }
    }
    wrefresh(win);
}

bool killProcess(int pid, WINDOW* statusWin) {
    if (kill(pid, SIGTERM) == 0) {
        werase(statusWin);
        wattron(statusWin, COLOR_PAIR(4));
        mvwprintw(statusWin, 0, 0, "SIGTERM sent to process %d", pid);
        wattroff(statusWin, COLOR_PAIR(4));
        wrefresh(statusWin);
        return true;
    }
    if (kill(pid, SIGKILL) == 0) {
        werase(statusWin);
        wattron(statusWin, COLOR_PAIR(4));
        mvwprintw(statusWin, 0, 0, "SIGKILL sent to process %d", pid);
        wattroff(statusWin, COLOR_PAIR(4));
        wrefresh(statusWin);
        return true;
    }
    werase(statusWin);
    wattron(statusWin, COLOR_PAIR(5));
    mvwprintw(statusWin, 0, 0, "Failed to kill process %d: %s", pid, strerror(errno));
    wattroff(statusWin, COLOR_PAIR(5));
    wrefresh(statusWin);
    return false;
}

int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    if (!has_colors()) {
        endwin();
        printf("Your terminal does not support color\n");
        return 1;
    }
    initColors();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    WINDOW* sysWin = newwin(7, cols / 2 - 1, 0, 0);
    WINDOW* procWin = newwin(rows - 7, cols, 7, 0);
    WINDOW* statusWin = newwin(1, cols, rows - 1, 0);

    int sortMode = 2;
    int selectedRow = 0;

    while (true) {
        werase(stdscr);
        refresh();

        displaySystemInfo(sysWin);

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

        displayProcessList(procWin, processes, selectedRow);

        werase(statusWin);
        wattron(statusWin, COLOR_PAIR(1));
        mvwprintw(statusWin, 0, 0, "Sort: [P]ID [C]PU [M]emory | [Up/Down] Select | [K]ill | [Q]uit");
        wattroff(statusWin, COLOR_PAIR(1));
        wrefresh(statusWin);

        int ch = getch();
        switch (ch) {
        case 'p':
        case 'P':
            sortMode = 0;
            break;
        case 'c':
        case 'C':
            sortMode = 1;
            break;
        case 'm':
        case 'M':
            sortMode = 2;
            break;
        case KEY_UP:
            if (selectedRow > 0) selectedRow--;
            break;
        case KEY_DOWN:
            if (selectedRow < (int)processes.size() - 1) selectedRow++;
            break;
        case 'k':
        case 'K':
            if (selectedRow >= 0 && selectedRow < (int)processes.size()) {
                killProcess(processes[selectedRow].pid, statusWin);
            }
            break;
        case 'q':
        case 'Q':
            delwin(sysWin);
            delwin(procWin);
            delwin(statusWin);
            endwin();
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return 0;
}
