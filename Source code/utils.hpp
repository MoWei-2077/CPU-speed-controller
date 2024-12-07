#pragma once
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <string>
#include <ctime>
#include <map>
#include <functional>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <errno.h>
#include <thread>
#include <sys/inotify.h>
#include <sys/mount.h>
#include "INIreader.hpp"
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
    
    bool FileWrite(const std::string& filePath, const std::string& content1, const std::string& content2) noexcept {
        int fd = open(filePath.c_str(), O_WRONLY | O_NONBLOCK);

        if (fd < 0) {
            chmod(filePath.c_str(), 0666);
            fd = open(filePath.c_str(), O_WRONLY | O_NONBLOCK); 
        }

         if (fd >= 0) {
            ssize_t bytesWritten1 = write(fd, content1.data(), content1.size());
            ssize_t bytesWritten2 = write(fd, content2.data(), content2.size());
            close(fd);
            chmod(filePath.c_str(), 0444);
        
            return (bytesWritten1 != -1 && bytesWritten2 != -1);
        }
        return false;
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
    size_t readString(const char* path, char* buff, const size_t maxLen) {
        auto fd = open(path, O_RDONLY);
        if (fd <= 0) {
            buff[0] = 0;
            return 0;
        }
        ssize_t len = read(fd, buff, maxLen);
        close(fd);
        if (len <= 0) {
            buff[0] = 0;
            return 0;
        }
        buff[len] = 0; // 终止符
        return static_cast<size_t>(len);
    }
    
    bool checkschedhorizon() {
        const std::string schedhorizon_path = "/sys/devices/system/cpu/cpufreq/policy0/schedhorizon";
        return access(schedhorizon_path.c_str(), F_OK) == 0;
    }
    
    std::string getPids(const std::vector<std::string>& processNames) {
        DIR* dir = opendir("/proc");
        if (dir == nullptr) {
            log("错误:无法打开/proc 目录");
            return "";
        }

        std::ostringstream Pids;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR && std::all_of(entry->d_name, entry->d_name + strlen(entry->d_name), ::isdigit)) {
                pid_t pid = static_cast<pid_t>(std::stoi(entry->d_name));
                std::string cmdlinePath = "/proc/" + std::string(entry->d_name) + "/cmdline";

                std::ifstream cmdlineFile(cmdlinePath);
                if (cmdlineFile) {
                    std::string cmdline;
                    std::getline(cmdlineFile, cmdline, '\0'); 
                    for (const auto& processName : processNames) {
                        if (cmdline.find(processName) != std::string::npos) {
                            Pids << pid << '\n';
                            break; 
                        }
                    }
                }
            }
        }
        closedir(dir);
        return Pids.str();
    }

    std::string getTids(const std::string& pids) {
        std::ostringstream Tids;
        std::istringstream iss(pids);
        std::string pid;

        while (std::getline(iss, pid, '\n')) {
            std::string taskDir = "/proc/" + pid + "/task";
            DIR* dir = opendir(taskDir.c_str());
            if (dir == nullptr) {
                log("错误:无法打开/proc/" + pid + "/task 目录");
                return "";
            }

            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_type == DT_DIR && std::all_of(entry->d_name, entry->d_name + strlen(entry->d_name), ::isdigit)) {
                    Tids << entry->d_name << '\n';
                }
            }
            closedir(dir);
        }
        return Tids.str();
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
