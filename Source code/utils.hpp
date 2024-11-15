#pragma once
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <string>
#include <ctime>
#include <map>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <errno.h>
#include <thread>
#include "INIreader.hpp"
#include <sys/inotify.h>

class Utils {
private:
    const std::string logpath = "/sdcard/Android/MW_CpuSpeedController/log.txt";

public:
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
   
    std::string exec(const std::string& command) {
        char buffer[128];
        std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            std::cerr << "popen() failed!" << std::endl;
            return "";
        }
        
        if (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
            result = buffer; 
        }

        return result;  
    }
    
    size_t popenRead(const char* cmd, char* buf, const size_t maxLen) {
        auto fp = popen(cmd, "r");
        if (!fp) return 0;
        auto readLen = fread(buf, 1, maxLen, fp);
        pclose(fp);
        return readLen;
    }
    
    void Init() {
        char buf[256] = { 0 };
        if (popenRead("pidof MW_CpuSpeedController", buf, sizeof(buf)) == 0) {
            log("进程检测失败");
            exit(-1);
        }

        auto ptr = strchr(buf, ' ');
        if (ptr) { // "pidNum1 pidNum2 ..."  如果存在多个pid就退出
            char tips[256];
            auto len = snprintf(tips, sizeof(tips),
                "警告: CS调度已经在运行 (pid: %s), 当前进程(pid:%d)即将退出", buf, getpid());
            printf("\n!!! \n!!! %s\n!!!\n\n", tips);
            printf(tips, len);
            exit(-2);
        }
    }
    
    bool checkschedhorizon() {
        const std::string schedhorizon_path = "/sys/devices/system/cpu/cpufreq/policy0/schedhorizon";
        return access(schedhorizon_path.c_str(), F_OK) == 0;
    }

    void Initschedhorizon() {
        if (checkschedhorizon()) {
            log("您的设备支持CS调度");
        } else {
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
