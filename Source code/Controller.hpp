#include <sys/inotify.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fcntl.h> 
#include <stdio.h>
#include <errno.h>
#include "utils.hpp"

class Controller {
private:
    constexpr std::string topPath = "/dev/cpuset/top-app/cgroup.procs";
    constexpr std::string configPath = "/sdcard/Android/MW_CpuSpeedController/config.txt";
    constexpr std::string getTopPackage = "dumpsys window | grep -i \"mCurrentFocus\" | awk -F'[/{]' '{print $2}' | cut -d '/' -f 1 | awk '{print $3}'";
    void mainControlFunction() {
        int status;
        std::string out;
        while (true) {
            status = monitorFileChanges(topPath, IN_MODIFY);
            if (status == 1) {
                //此时前台变化
                out = runCommand(getTopPackage);
                utils.log("前台变化为:" + out);
            }
        }
    }

    // 监听前台的变化
    int monitorFileChanges(const std::string_view& path, unsigned int mask) {
        int fd; // 文件描述符
        char buffer[1024]; // 缓冲区
        struct inotify_event* event; // 事件结构体
        int wd; // 监视描述符

        // 初始化 inotify 实例
        fd = inotify_init();
        if (fd == -1) {
            return -1;
        }

        // 添加监视的文件或目录
        wd = inotify_add_watch(fd, path.data(), mask);
        if (wd == -1) {
            close(fd);
            return -1;
        }

        while (true) {
            // 读取 inotify 事件
            int len = read(fd, buffer, sizeof(buffer));
            if (len == -1) {
                close(fd);
                return -1;
            }

            char* ptr = buffer;
            while (ptr < buffer + len) {
                event = (struct inotify_event*)ptr;

                // 检测到 IN_MODIFY 事件后返回 1
                if ((event->mask & IN_MODIFY) != 0) {
                    close(fd);
                    return 1;
                }

                ptr += sizeof(struct inotify_event) + event->len;
            }
        }

        // 清理资源
        close(fd);
        return 0; // 不应到达这里
    }

    void writeConfig(std::string mode) {
        runCommand("su -c echo '" + mode + "' > " + std::string(configPath));
    }

    std::string runCommand(const std::string& command) {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return "";
        }

        std::string result;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }

        pclose(pipe);
        return result;
    }

public:

    void bootFunction() {
        mainControlFunction();
    }
};
Utils utils;