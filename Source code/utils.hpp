#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <thread>
#include <map>
#include <sstream>
#include <thread>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <filesystem>
#include "INIreader.hpp"
#include <sys/inotify.h>

class Utils {
private:
    const std::string logpath = "/sdcard/Android/MW_CpuSpeedController/log.txt";

public:
    const char* cpusetEventPath = "/dev/cpuset/top-app";
    //CS_Speed csspeed;
    void clear_log() {
        std::ofstream ofs;
        ofs.open(logpath, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }
    void log(const std::string& message) {
        std::time_t now = std::time(nullptr);
        std::tm* local_time = std::localtime(&now);
        char time_str[100];
        std::strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S]", local_time);

        std::ofstream logfile(logpath, std::ios_base::app);
        if (logfile.is_open()) {
            logfile << time_str << " " << message << std::endl;
            logfile.close();
        }
    }


    void Init() {
        std::string processName = "MW_CpuSpeedController";
        std::string command = "pidof " + processName;
        FILE* pipe = popen(command.c_str(), "r");

        if (!pipe) {
            log("错误: 进程检测失败，正在退出进程");
            exit(1);
        }

        int count = 0;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            count++;
        }

        pclose(pipe);

        if (count > 2) {
            log("警告: CS调度已经在运行 pids: " + processName + " 当前进程(pid:%d)即将退出");
            log("警告: 请勿手动启动CS调度，也不要在多个框架同时启动CS调度");
            exit(1);
        }
    }

    bool checkconf() {
        const std::string conf_path = "/sdcard/Android/MW_CpuSpeedController/config.ini";
        return access(conf_path.c_str(), F_OK) == 0;
    }
    void Initconf() {
        if (checkconf()) {
           //
        } else {
            log("配置文件不存在，请重新刷入CS调度");
        }
    }
    bool checkschedhorizon() {
        const std::string schedhorizon_path = "/sys/devices/system/cpu/cpufreq/policy0/schedhorizon";
        return access(schedhorizon_path.c_str(), F_OK) == 0;
    }

    void Initschedhorizon() {
        if (checkschedhorizon()) {
            log("您的设备支持CS调度");
        }
        else {
            log("警告:您的设备不支持CS调度 CS调度进程已退出");
            exit(1);
        }
    }
    int InotifyMain(const char* dir_name, uint32_t mask) {
        int fd = inotify_init();
        if (fd < 0) {
            std::cerr << "Failed to initialize inotify." << std::endl;
            return -1;
        }

        int wd = inotify_add_watch(fd, dir_name, mask);
        if (wd < 0) {
            std::cerr << "Failed to add watch for directory: " << dir_name << std::endl;
            close(fd);
            return -1;
        }

        const int buflen = sizeof(struct inotify_event) + NAME_MAX + 1;
        char buf[buflen];
        fd_set readfds;

        while (true) {
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);

            int iRet = select(fd + 1, &readfds, nullptr, nullptr, nullptr);
            if (iRet < 0) {
                break;
            }

            int len = read(fd, buf, buflen);
            if (len < 0) {
                std::cerr << "Failed to read inotify events." << std::endl;
                break;
            }

            const struct inotify_event* event = reinterpret_cast<const struct inotify_event*>(buf);
            if (event->mask & mask) {
                break;
            }
        }

        inotify_rm_watch(fd, wd);
        close(fd);

        return 0;
    }
};
