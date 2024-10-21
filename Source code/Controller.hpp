#pragma once
#include <sys/inotify.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fcntl.h> 
#include <stdio.h>
#include <errno.h>
#include <vector>
#include <fstream>
#include "utils.hpp"
#include "CS_Speed.hpp"

class Controller {
private:
    Utils utils;
    const std::string topPath = "/dev/cpuset/top-app/cgroup.procs";
    const std::string configPath = "/sdcard/Android/MW_CpuSpeedController/config.txt";
    const std::string getTopPackage = "dumpsys window | grep -i \"mCurrentFocus\" | awk -F'[/{]' '{print $2}' | cut -d '/' -f 1 | awk '{print $3}'";
    
    CS_Speed csspeed;

    FILE* pipe;

    const std::string bPath = "/sdcard/Android/CSController/balance.txt";
    const std::string poPath = "/sdcard/Android/CSController/powersave.txt";
    const std::string pePath = "/sdcard/Android/CSController/performance.txt";
    const std::string faPath = "/sdcard/Android/CSController/fast.txt";

    std::vector<std::string> blist;
    std::vector<std::string> polist;
    std::vector<std::string> pelist;
    std::vector<std::string> falist;

    std::vector<std::string> readPackageNamesFromPipe(FILE* pipe) {
        if (!pipe) {
            return {};
        }

        std::string result;
        char buffer[2048];
        std::vector<std::string> packageNames;

        
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }

        
        size_t start = 0;
        size_t end = result.find('\n');
        while (end != std::string::npos) {
            
            packageNames.push_back(result.substr(start, end - start));
            start = end + 1; 
            end = result.find('\n', start);
        }

        
        if (start < result.size()) {
            packageNames.push_back(result.substr(start, std::string::npos));
        }

        pclose(pipe);
        return packageNames;
    }

    void updateLists() {
        for (int i = 0; i <= 3; i++) {
            switch (i)
            {
            case 0:
                pipe = popen("su -c cat /sdcard/Android/CSController/powersave.txt", "r");
                polist.clear();
                polist = readPackageNamesFromPipe(pipe);
                break;
            case 1:
                pipe = popen("su -c cat /sdcard/Android/CSController/balance.txt", "r");
                blist.clear();
                blist = readPackageNamesFromPipe(pipe);
                break;
            case 2:
                pipe = popen("su -c cat /sdcard/Android/CSController/performance.txt", "r");
                pelist.clear();
                pelist = readPackageNamesFromPipe(pipe);
                break;
            case 3:
                pipe = popen("su -c cat /sdcard/Android/CSController/fast.txt", "r");
                falist.clear();
                falist = readPackageNamesFromPipe(pipe);
                break;
            default:
                break;
            }
        }
    }

    bool containsSubstring(const std::string& subStr, const std::vector<std::string>& strVec) {
        for (const auto& str : strVec) {
            if (str.find(subStr) != std::string::npos) {
                return true;  // ���ַ�������
            }
        }
        return false;  // ���ַ������������κ�Ԫ����
    }

    void mainControlFunction() {
        int status;
        std::string out;
        while (true) {
            status = monitorFileChanges(topPath, IN_MODIFY);
            if (status == 1) {
                //��ʱǰ̨�仯
                out = runCommand(getTopPackage);
                if (out == " " || out == "")continue;
                if (containsSubstring(out, polist)) {
                    csspeed.powersave();
                    utils.log("ģʽ�л�Ϊpowersave");
                }
                else if (containsSubstring(out, blist)) {
                    csspeed.balance();
                    utils.log("ģʽ�л�Ϊbalance");
                }
                else if (containsSubstring(out, pelist)) {
                    csspeed.performance();
                    utils.log("ģʽ�л�Ϊperformance");
                }
                else if (containsSubstring(out, falist)) {
                    csspeed.fast();
                    utils.log("ģʽ�л�Ϊfast");
                }
                else {
                    continue;
                }
            }
        }
    }

    // ����ǰ̨�ı仯
    int monitorFileChanges(const std::string_view& path, unsigned int mask) {
        int fd; // �ļ�������
        char buffer[1024]; // ������
        struct inotify_event* event; // �¼��ṹ��
        int wd; // ����������

        // ��ʼ�� inotify ʵ��
        fd = inotify_init();
        if (fd == -1) {
            return -1;
        }

        // ��Ӽ��ӵ��ļ���Ŀ¼
        wd = inotify_add_watch(fd, path.data(), mask);
        if (wd == -1) {
            close(fd);
            return -1;
        }

        while (true) {
            // ��ȡ inotify �¼�
            int len = read(fd, buffer, sizeof(buffer));
            if (len == -1) {
                close(fd);
                return -1;
            }

            char* ptr = buffer;
            while (ptr < buffer + len) {
                event = (struct inotify_event*)ptr;

                // ��⵽ IN_MODIFY �¼��󷵻� 1
                if ((event->mask & IN_MODIFY) != 0) {
                    close(fd);
                    return 1;
                }

                ptr += sizeof(struct inotify_event) + event->len;
            }
        }

        // ������Դ
        close(fd);
        return 0; // ��Ӧ��������
    }

    void writeConfig(std::string mode) {
        runCommand("su -c echo '" + mode + "' > " + std::string(configPath));
    }

    std::string runCommand(const std::string& command) {
        pipe = popen(command.c_str(), "r");
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