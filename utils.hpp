#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sstream>
#include <numeric>
#include <regex>
#include <dirent.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include "json.hpp"

  const std::string Log_path = "/sdcard/Android/MW_CpuSpeedController/log.txt"; // 日志文件
  const std::string config_path = "/sdcard/Android/MW_CpuSpeedController/config.txt"; 
  const std::string json_path = "/sdcard/Android/MW_CpuSpeedController/config.json"; // 后续的配置文件
    /*接下来都是针对CPU调度的路径*/
  const std::string cpu_uclamp_min = "/dev/cpuctl/top-app/cpu.uclamp.min";
  const std::string cpu_uclamp_max = "/dev/cpuctl/top-app/cpu.uclamp.max";
  const std::string foreground_cpu_uclamp_min = "/dev/cpuctl/foreground/cpu.uclamp.min";
  const std::string foreground_cpu_uclamp_max = "/dev/cpuctl/foreground/cpu.uclamp.max";
  const std::string cpu_min_freq = "/sys/kernel/msm_performance/parameters/cpu_min_freq";
  // 下面都是核心绑定
  const std::string background_cpu = "/dev/cpuset/background/cpus"; // 用户的后台应用
  const std::string system_background_cpu = "/dev/cpuset/system-background/cpus"; // 系统的后台应用
  const std::string foreground_cpu = "/dev/cpuset/foreground/cpus"; // 前台的应用
  const std::string top_app = "/dev/cpuset/top-app/cpus"; // 顶层应用
  using json = nlohmann::json;
  nlohmann::json data;

inline void Log(const std::string& message) {
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S]", local_time);

    std::ofstream logfile(Log_path, std::ios_base::app);
    if (logfile.is_open()) {
        logfile << time_str << " " << message << std::endl;
        logfile.close();
    }
}
  std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}
void readjsonFile(const std::string& path, nlohmann::json& data) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }
    file >> data;  // 使用 nlohmann::json 的 >> 操作符来解析 JSON
    file.close();
}
// 去除字符串两端的空白字符
std::string TrimStr(std::string str) {
    str.erase(0, str.find_first_not_of(' ')); // 前导空格
    str.erase(str.find_last_not_of(' ') + 1); // 尾随空格
    return str;
}
void Init() {
    std::string processName = "MW_CpuSpeedController";
    std::string command = "pidof " + processName;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        Log("错误:进程检测失败 正在退出进程");
        exit(1);
    }

    int count = 0;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        count++;
    }

    pclose(pipe);

    if (count > 2) {
        Log("警告:CS调度已经在运行 pids:" + processName + " 当前进程(pid:%d)即将退出");
        Log("警告:请勿手动启动CS调度, 也不要在多个框架同时启动CS调度");
        exit(1);
    }
}
bool checkschedhorizon(){
    const std::string schedhorizon_path = "/sys/devices/system/cpu/cpufreq/policy0/schedhorizon";
    return access(schedhorizon_path.c_str(), F_OK) == 0;
}
inline void Initschedhorizon(){
    if (checkschedhorizon()) {
        Log("您的设备支持CS调度");
    } else {
        Log("警告:您的设备不支持CS调度 CS调度进程已退出");
        exit(1);
    }
}
inline void WriteFile(const std::string &filePath, const std::string &content) noexcept
{
    int fd = open(filePath.c_str(), O_WRONLY | O_NONBLOCK);

    if (fd < 0) {
        chmod(filePath.c_str(), 0666);
        fd = open(filePath.c_str(), O_WRONLY | O_NONBLOCK); // 如果写入失败将会授予0666权限
    }

    if (fd >= 0) {
        write(fd, content.data(), content.size());
        close(fd); // 写入成功后关闭文件 防止调速器恢复授予0444权限
        chmod(filePath.c_str(), 0444);
    }
}
inline void disable_qcomGpuBoost(){
    std::string num_pwrlevels_path = "/sys/class/kgsl/kgsl-3d0/num_pwrlevels";
    const std::string quomGPU_path = "/sys/class/kgsl/kgsl-3d0/";
    std::ifstream file(num_pwrlevels_path);
    int num_pwrlevels;
    if (file >> num_pwrlevels) {
        int MIN_PWRLVL = num_pwrlevels - 1;
        std::string minPwrlvlStr = std::to_string(MIN_PWRLVL);
        Log("已关闭高通GPU Boost");
        WriteFile(quomGPU_path + "default_pwrlevel", minPwrlvlStr);
        WriteFile(quomGPU_path + "min_pwrlevel", minPwrlvlStr);
        WriteFile(quomGPU_path + "max_pwrlevel", "0");
        WriteFile(quomGPU_path + "thermal_pwrlevel", "0");   
        WriteFile(quomGPU_path + "throttling", "0");
    }
}
inline int InotifyMain(const char *dir_name, uint32_t mask) {
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

        const struct inotify_event *event = reinterpret_cast<const struct inotify_event *>(buf);
        if (event->mask & mask) {
            break;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);

    return 0;
}/*
int InotifyTouch() {
    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "Failed to initialize inotify." << std::endl;
        return -1;
    }

    // 遍历 /dev/input/ 目录
    DIR *dir = opendir("/dev/input");
    if (!dir) {
        std::cerr << "Failed to open /dev/input." << std::endl;
        close(fd);
        return -1;
    }

    // 存储 watch descriptors
    std::vector<int> watches;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && strstr(entry->d_name, "event")) {
            std::string path = "/dev/input/";
            path += entry->d_name;

            int wd = inotify_add_watch(fd, path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
            if (wd < 0) {
                std::cerr << "Failed to add watch for directory: " << path << std::endl;
                continue;
            }
            watches.push_back(wd);
        }
    }
    closedir(dir);

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

        const struct inotify_event *event = reinterpret_cast<const struct inotify_event *>(buf);
        if (event->mask & (IN_MODIFY | IN_CREATE | IN_DELETE)) {
           //这里触摸发生了变化
            balance_mode();
        }
    }

    // 移除所有监控
    for (auto wd : watches) {
        inotify_rm_watch(fd, wd);
    }

    close(fd);
    
    return 0;
}*/
std::vector<std::string> GpuDDR() {
    std::vector<std::string> tripPointPaths;
    std::regex cpuThermalRegex("GPU|DDR");

    auto dir = opendir("/sys/class/thermal");
    if (dir) {
        for (auto entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
            std::string dirName(entry->d_name);
            if (dirName.find("thermal_zone") != std::string::npos) {
                auto type = TrimStr(ReadFile("/sys/class/thermal/" + dirName + "/type"));
                if (std::regex_match(type, cpuThermalRegex)) {
                    DIR *zoneDir = opendir(("/sys/class/thermal/" + dirName).c_str());
                    if (zoneDir) {
                        for (auto zoneEntry = readdir(zoneDir); zoneEntry != nullptr; zoneEntry = readdir(zoneDir)) {
                            std::string fileName(zoneEntry->d_name);
                            if (fileName.find("trip_point_0_temp") == 0) {
                                std::string tripPointPath = "/sys/class/thermal/" + dirName + "/" + fileName;
                                tripPointPaths.push_back(tripPointPath);
                            }
                        }
                        closedir(zoneDir);
                    }
                    break;
                }
            }
        }
        closedir(dir);
    }
    return tripPointPaths;
}
inline void core_allocation() {
    WriteFile(background_cpu, "0-1");
    WriteFile(system_background_cpu, "0-2");
    WriteFile(foreground_cpu, "0-6");
    WriteFile(top_app, "0-7");
}
inline void enableFeas(){
    Log("Feas已启用"); // 恢复原本参数 让系统开启官调
    WriteFile(cpu_uclamp_min, "20");
    WriteFile(cpu_uclamp_max, "max");
    WriteFile(foreground_cpu_uclamp_min, "10");
    WriteFile(foreground_cpu_uclamp_max, "80");
    for (int i = 0; i <= 7; ++i) {
        std::string cpuDir = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/";
        WriteFile(cpuDir + "scaling_max_freq", "2147483647"); 
        WriteFile(cpuDir + "scaling_min_freq", "0");  
        WriteFile(cpuDir + "scaling_governor", "sugov_ext");  
        WriteFile(cpuDir + "scaling_governor", "walt");  
    }
}
inline void schedhorizon(){
    for (int i = 0; i <= 7; ++i) {
        std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
        WriteFile(cpuDir, "schedhorizon");  
    }
}
inline void super_powersave_mode(){
    Log("更换模式为超级省电模式");
    schedhorizon();
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/efficient_freq", "1700000");
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/up_delay", "70");
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", "600000");
    WriteFile("/sys/devices/system/cpu/cpu0/cpufreq/schedhorizon/down_rate_limit_us", "2000");
    WriteFile("/sys/devices/system/cpu/cpu0/cpufreq/schedhorizon/up_rate_limit_us", "6000");

    const std::string frequencies = "1400000 1700000 2000000 2500000";
    const std::string efficient_freq = "200 200 300 500";
    for (int i = 1; i <= 5; ++i) {
      const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
      const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
      const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
      const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "cpufreq/schedhorizon/down_rate_limit_us";
      const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "cpufreq/schedhorizon/up_rate_limit_us";
        WriteFile(filePath, frequencies);
        WriteFile(up_delayPath, efficient_freq);
        WriteFile(scaling_min_freq_limit_path, "600000");
        WriteFile(down_rate_limit_us_path, "2000");
        WriteFile(up_rate_limit_us_path, "6000");
    }

    const std::string frequencies6_7 = "1200000 1800000 2500000";
    const std::string efficient_freq6_7 = "150 500 500";
    for (int i = 6; i <= 7; ++i) {
      const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
      const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
      const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
      const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "cpufreq/schedhorizon/down_rate_limit_us";
      const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "cpufreq/schedhorizon/up_rate_limit_us";
        WriteFile(filePath6_7, frequencies6_7);
        WriteFile(up_delayPath6_7, efficient_freq6_7);
        WriteFile(scaling_min_freq_limit_path6_7, "600000");
        WriteFile(down_rate_limit_us_path6_7, "2000");
        WriteFile(up_rate_limit_us_path6_7, "6000");
    }


WriteFile("/dev/cpuctl/top-app/cpu.uclamp.min", "10");
WriteFile("/dev/cpuctl/top-app/cpu.uclamp.max", "50");
WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "10");
WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "50");


WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "true");
    std::vector<std::string> paths = GpuDDR();
    for (const auto& path : paths) {
        WriteFile(path, "45000"); // 45℃ 
    }
}
inline void powersave_mode(){
    Log("更换模式为省电模式");
    schedhorizon();
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/efficient_freq", "1700000");
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/up_delay", "50");
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", "900000");
     WriteFile("/sys/devices/system/cpu/cpu0/cpufreq/schedhorizon/down_rate_limit_us", "1500");
    WriteFile("/sys/devices/system/cpu/cpu0/cpufreq/schedhorizon/up_rate_limit_us", "1000");
    const std::string frequencies = "1400000 1700000 2000000 2500000";
    const std::string efficient_freq = "100 150 300 500";
    for (int i = 1; i <= 5; ++i) {
      const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
      const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
      const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
      const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
      const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
        WriteFile(filePath, frequencies);
        WriteFile(up_delayPath, efficient_freq);
        WriteFile(scaling_min_freq_limit_path, "700000");
        WriteFile(down_rate_limit_us_path, "1500");
        WriteFile(up_rate_limit_us_path, "1000");
    }

    const std::string frequencies6_7 = "1200000 1800000 2500000";
    const std::string efficient_freq6_7 = "100 300 500";
    for (int i = 6; i <= 7; ++i) {
      const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
      const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
      const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
      const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
      const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
        WriteFile(filePath6_7, frequencies6_7);
        WriteFile(up_delayPath6_7, efficient_freq6_7);
        WriteFile(scaling_min_freq_limit_path6_7, "900000");
        WriteFile(down_rate_limit_us_path6_7, "1500");
        WriteFile(up_rate_limit_us_path6_7, "1000");
    }

WriteFile("/dev/cpuctl/top-app/cpu.uclamp.min", "10");
WriteFile("/dev/cpuctl/top-app/cpu.uclamp.max", "75");
WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "10");
WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "65");
WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "true");
    std::vector<std::string> paths = GpuDDR();
    for (const auto& path : paths) {
        WriteFile(path, "60000"); // 60℃ 
    }
}
inline void balance_mode(){
    Log("更换模式为均衡模式");
    schedhorizon();

    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/efficient_freq", "1700000");
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/up_delay", "50");
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", "1200000");
    WriteFile("/sys/devices/system/cpu/cpu0/cpufreq/schedhorizon/down_rate_limit_us", "1000");
    WriteFile("/sys/devices/system/cpu/cpu0/cpufreq/schedhorizon/up_rate_limit_us", "900");
    const std::string frequencies = "1400000 1700000 2000000 2500000";
    const std::string efficient_freq = "50 100 150 200";
    for (int i = 1; i <= 5; ++i) {
      const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
      const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
      const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
      const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
      const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
        WriteFile(filePath, frequencies);
        WriteFile(up_delayPath, efficient_freq);
        WriteFile(scaling_min_freq_limit_path, "1200000");
        WriteFile(down_rate_limit_us_path, "1000");
        WriteFile(up_rate_limit_us_path, "900");
    }

    const std::string frequencies6_7 = "1200000 1800000 2500000";
    const std::string efficient_freq6_7 = "100 200 300";
    for (int i = 6; i <= 7; ++i) {
      const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
      const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
      const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
      const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
      const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
        WriteFile(filePath6_7, frequencies6_7);
        WriteFile(up_delayPath6_7, efficient_freq6_7);
        WriteFile(scaling_min_freq_limit_path6_7, "1400000");
        WriteFile(down_rate_limit_us_path6_7, "1000");
        WriteFile(up_rate_limit_us_path6_7, "900");
    }
WriteFile("/dev/cpuctl/top-app/cpu.uclamp.min", "10");
WriteFile("/dev/cpuctl/top-app/cpu.uclamp.max", "80");
WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "10");
WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "75");
WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "true");
    std::vector<std::string> paths = GpuDDR();
    for (const auto& path : paths) {
        WriteFile(path, "75000"); // 75℃ 
    }
}


inline void performance_mode(){
    Log("更换模式为性能模式");
    schedhorizon();

    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/efficient_freq", "1700000");
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/up_delay", "10");
    WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", "1800000");
    WriteFile("/sys/devices/system/cpu/cpu0/cpufreq/schedhorizon/down_rate_limit_us", "800");
    WriteFile("/sys/devices/system/cpu/cpu0/cpufreq/schedhorizon/up_rate_limit_us", "500");
    const std::string frequencies = "1400000 1700000 2000000 2500000";
    const std::string efficient_freq = "50 50 100 150";
    for (int i = 1; i <= 5; ++i) {
      const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
      const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
      const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
      const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
      const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us"; 
        WriteFile(filePath, frequencies);
        WriteFile(up_delayPath, efficient_freq);
        WriteFile(scaling_min_freq_limit_path, "1600000");
        WriteFile(down_rate_limit_us_path, "800");
        WriteFile(up_rate_limit_us_path, "500");
    }

    const std::string frequencies6_7 = "1200000 1800000 2500000";
    const std::string efficient_freq6_7 = "50 100 150";
    for (int i = 6; i <= 7; ++i) {
      const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
      const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
      const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
      const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
      const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
        WriteFile(filePath6_7, frequencies6_7);
        WriteFile(up_delayPath6_7, efficient_freq6_7);
        WriteFile(scaling_min_freq_limit_path6_7, "1800000");
        WriteFile(down_rate_limit_us_path6_7, "800");
        WriteFile(up_rate_limit_us_path6_7, "500");
    }
WriteFile("/dev/cpuctl/top-app/cpu.uclamp.min", "10");
WriteFile("/dev/cpuctl/top-app/cpu.uclamp.max", "max");
WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "10");
WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "max");

    WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "true");
    std::vector<std::string> paths = GpuDDR();
    for (const auto& path : paths) {
        WriteFile(path, "100000"); // 100℃ 
    }
}
inline void fast_mode(){
 /*   std::ifstream file(json_path);
    json data;
    // 将文件内容解析为json对象
    file >> data;
if (data.contains("Enable_Feas") && data["Enable_Feas"] == true) {
    enableFeas();
} 
if (data.contains("Enable_Feas") && data["Enable_Feas"] == false) {
        Log("更换模式为极速模式");
for (int i = 0; i <= 7; ++i) {
    std::string cpuDir = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon";
      const std::string policy_PATH = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i);
     WriteFile(policy_PATH + "/scaling_min_freq_limit", "1800000");
     WriteFile(cpuDir + "/down_rate_limit_us", "1000");
     WriteFile(cpuDir + "/up_rate_limit_us", "300");
}
	WriteFile(cpu_uclamp_min, "0");
	WriteFile(cpu_uclamp_max, "max");
	WriteFile(foreground_cpu_uclamp_min, "0");
	WriteFile(foreground_cpu_uclamp_max, "90");
    WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "false");
    std::vector<std::string> paths = GpuDDR();
    for (const auto& path : paths) {
        WriteFile(path, "130000"); // 130℃ 
        }
    }      
    */
   enableFeas();
}
inline void Getconfig(const std::string& config_path) {
    std::ifstream file(config_path);
    std::string line;
    while (std::getline(file, line)) { // 按行读取文件
        if (line == "powersave") {
            powersave_mode();
        } else if (line == "balance") {
            balance_mode();
        } else if (line == "performance") {
            performance_mode();
        } else if (line == "fast") {
            fast_mode();
        }  else if (line == "super_powersave") {
            super_powersave_mode();
       }
   }
}
inline void clear_log(){
	std::ofstream ofs;
    ofs.open(Log_path, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}
inline void Getjson(){
    std::ifstream file(json_path);
    json data;

    // 将文件内容解析为json对象
    file >> data;

    // 检查json中的特定键是否存在，并根据其值决定是否调用函数
    if (data.contains("Disable_qcom_GpuBoost") && data["Disable_qcom_GpuBoost"] == true) {
         disable_qcomGpuBoost();
    } 
    if (data.contains("Core_allocation") && data["Core_allocation"] == true) {
         core_allocation();
    } 
}