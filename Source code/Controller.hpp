#pragma once
#include "CS_Speed.hpp"
#include "utils.hpp"

class Controller {
private:
    Utils utils;
    
        
    CS_Speed csspeed;
    FILE* pipe;
    const std::string configPath = "/sdcard/Android/MW_CpuSpeedController/config.txt";
    const std::string getTopPackage = "dumpsys window | grep -i \"mCurrentFocus\" | awk -F'[/{]' '{print $2}' | cut -d '/' -f 1 | awk '{print $3}'";


    const std::string bPath = "/sdcard/Android/CSController/balance.txt";
    const std::string poPath = "/sdcard/Android/CSController/powersave.txt";
    const std::string pePath = "/sdcard/Android/CSController/performance.txt";
    const std::string faPath = "/sdcard/Android/CSController/fast.txt";
    std::vector<std::thread> threads;
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
                return true;  // 子字符串存在
            }
        }
        return false;   // 子字符串不存在于任何元素中
    }

    void mainControlFunction() {
        std::string out;
        while (true) {
            utils.log("启动成功");
            utils.InotifyMain("/dev/cpuset/top-app/cgroup.procs", IN_MODIFY);
            // 自动堵塞
            out = runCommand(getTopPackage);
            if (out == " " || out == "")continue;
            if (containsSubstring(out, polist)) {
                csspeed.powersave();
            }
            else if (containsSubstring(out, blist)) {
                csspeed.balance();
            }
            else if (containsSubstring(out, pelist)) {
                csspeed.performance();
            }
            else if (containsSubstring(out, falist)) {
                csspeed.fast();
            }
            else {
                continue;
            }
        }
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
        if (csspeed.Dynamic_response){
            threads.emplace_back(std::thread(&Controller::mainControlFunction, this)).detach();
        }
    }
};