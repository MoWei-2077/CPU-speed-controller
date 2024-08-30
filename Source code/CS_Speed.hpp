#pragma once
#include "utils.hpp"
#include "config.hpp"

class CS_Speed {
private:
    std::string name;
    std::string author;
    bool enableFeas;
    bool disableGpuBoost;
    bool coreAllocation;
    bool appFrequencyUpgrade;
    Utils utils;
    std::vector<std::thread> threads;
    INIReader reader;
    std::unordered_map<std::string, std::unordered_map<int, int>> frequencyLevels;
    const std::string schedhorizon_path = "/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/";
    const std::string cpu_uclamp_min = "/dev/cpuctl/top-app/cpu.uclamp.min";
    const std::string cpu_uclamp_max = "/dev/cpuctl/top-app/cpu.uclamp.max";
    const std::string foreground_cpu_uclamp_min = "/dev/cpuctl/foreground/cpu.uclamp.min";
    const std::string foreground_cpu_uclamp_max = "/dev/cpuctl/foreground/cpu.uclamp.max";
    const std::string background_cpu = "/dev/cpuset/background/cpus"; // 用户的后台应用
    const std::string system_background_cpu = "/dev/cpuset/system-background/cpus"; // 系统的后台应用
    const std::string foreground_cpu = "/dev/cpuset/foreground/cpus"; // 前台的应用
    const std::string top_app = "/dev/cpuset/top-app/cpus"; // 顶层应用
public:
    CS_Speed() : reader("/sdcard/Android/MW_CpuSpeedController/config.ini") {}
    void AppFrequencyUpgrade(){
        if (appFrequencyUpgrade) {  
            threads.emplace_back(std::thread(&CS_Speed::cpuSetTriggerTask, this)).detach();
        }
    }    
    void readAndParseConfig() {
         INIReader 
         reader("/sdcard/Android/MW_CpuSpeedController/config.ini");

        if (reader.ParseError() < 0) {
            std::cout << "Can't load 'config.ini'\n";
            return;
        }

        name = reader.Get("meta", "name", "");
        author = reader.Get("meta", "author", "");
        enableFeas = reader.GetBoolean("meta", "Enable_Feas", false);
        disableGpuBoost = reader.GetBoolean("meta", "Disable_qcom_GpuBoost", false);
        coreAllocation = reader.GetBoolean("meta", "Core_allocation", false);
        appFrequencyUpgrade = reader.GetBoolean("meta", "APP_freq_uency_upgrade", false);
    }
    void readFrequencyTable(const std::string& policy) {
        std::string filePath = "/sys/devices/system/cpu/cpufreq/" + policy + "/scaling_available_frequencies";
        std::ifstream file(filePath);

        std::string line;
        getline(file, line);
        file.close();

        std::vector<int> frequencies;
        size_t pos = 0;
        while ((pos = line.find(' ')) != std::string::npos) {
            std::string freqStr = line.substr(0, pos);
            int freq = stoi(freqStr);
            frequencies.push_back(freq);
            line.erase(0, pos + 1);
        }

    // Determine frequency levels
            int level = 1;
            int step = frequencies.size() / 12;
        for (int i = 0; i < frequencies.size(); i++) {
            frequencyLevels[policy][frequencies[i]] = level;
            if ((i + 1) % step == 0 && level < 15) {
                level++;
            }
        }
    }
    void setMaxFrequency(const std::string& policy, int level) {
       for (const auto& pair : frequencyLevels[policy]) {
            if (pair.second == level) {
                std::string maxFreqPath = "/sys/devices/system/cpu/cpufreq/" + policy + "/scaling_max_freq";
                WriteFile(maxFreqPath, std::to_string(pair.first));
                break;
            }
        }
    }

    void cpuSetTriggerTask() {
        constexpr int TRIGGER_BUF_SIZE = 8192;

        sleep(1);

        int inotifyFd = inotify_init();
        if (inotifyFd < 0) {
            fprintf(stderr, "错误:同步事件: 0xB1 (1/3)失败: [%d]:[%s]", errno, strerror(errno));
            exit(-1);
        }

        int watch_d = inotify_add_watch(inotifyFd, utils.cpusetEventPath, IN_MODIFY);

        if (watch_d < 0) {
            fprintf(stderr, "错误:同步事件: 0xB1 (2/3)失败: [%d]:[%s]", errno, strerror(errno));
            exit(-1);
        }

        utils.log("应用任务切换监听成功");

        char buf[TRIGGER_BUF_SIZE];


        while (read(inotifyFd, buf, TRIGGER_BUF_SIZE) > 0) {
                APPfrequencyupgrade();
        }
        inotify_rm_watch(inotifyFd, watch_d);
        close(inotifyFd);

        utils.log("警告:已退出监控同步事件: 0xB0");
    }
    void WriteFile(const std::string& filePath, const std::string& content) noexcept {
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
    void APPfrequencyupgrade() {
        performance();
        sleep(3);
        powersave();
    }
    void Performance(){
         for (int i = 0; i <= 7; ++i) {
            std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
            WriteFile(cpuDir, "performance");
        }
    }
    void schedhorizon() {
        for (int i = 0; i <= 7; ++i) {
            std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
            WriteFile(cpuDir, "schedhorizon");
        }
    }
    bool checkFeas() {
        const std::string Feas_path = "/sys/module/perfmgr/parameters/perfmgr_enable";
        return access(Feas_path.c_str(), F_OK) == 0;
    }
    void Feasdisable() {
        WriteFile("/sys/module/perfmgr/parameters/perfmgr_enable", "0");
    }
    void EnableFeas() {
        if (checkFeas()) {
            utils.log("Feas已启用"); // Feas已启用
            WriteFile("/sys/module/perfmgr/parameters/perfmgr_enable", "1");
        }
        else {
            utils.log("警告:您的设备不支持Feas 请检查您设备是否拥有Perfmgr模块 由于您的设备没有Perfmgr模块目前将不会调整任何模式请更换模式");        
        }
    }
    void powersave() {
        utils.log("省电模式已启用");
        schedhorizon();

        WriteFile(schedhorizon_path + "efficient_freq", "1400000");
        WriteFile(schedhorizon_path + "up_delay", "60");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1500");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "3000");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */
        const std::string frequencies = "1200000 1500000 1800000 2300000";
        const std::string efficient_freq = "200 250 300 500";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);

            WriteFile(down_rate_limit_us_path, "1500");
            WriteFile(up_rate_limit_us_path, "3000");
        }

        const std::string frequencies6_7 = "1200000 1600000 2000000";
        const std::string efficient_freq6_7 = "150 500 500";
        for (int i = 6; i <= 7; ++i) {
            const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            WriteFile(filePath6_7, frequencies6_7);
            WriteFile(up_delayPath6_7, efficient_freq6_7);
            WriteFile(down_rate_limit_us_path6_7, "1500");
            WriteFile(up_rate_limit_us_path6_7, "3000");
        }
    for (int i = 0; i <= 7; ++i) {
        std::string policy = "policy" + std::to_string(i);
        readFrequencyTable(policy);
        setMaxFrequency(policy, 7); 
    }
    }
    void balance() {
        utils.log("均衡模式已启用");
        schedhorizon();
        WriteFile(schedhorizon_path + "efficient_freq", "1700000");
        WriteFile(schedhorizon_path + "up_delay", "50");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1500");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "1000");

        const std::string frequencies = "1400000 1700000 2000000 2500000";
        const std::string efficient_freq = "150 200 300 500";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            const std::string scaling_min_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq";
            const std::string scaling_max_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_max_freq";
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);

            WriteFile(down_rate_limit_us_path, "1500");
            WriteFile(up_rate_limit_us_path, "1000");
        }

        const std::string frequencies6_7 = "1200000 1800000 2500000";
        const std::string efficient_freq6_7 = "150 500 500";
        for (int i = 6; i <= 7; ++i) {
            const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            const std::string scaling_min_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq";
            const std::string scaling_max_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_max_freq";
            WriteFile(filePath6_7, frequencies);
            WriteFile(up_delayPath6_7, efficient_freq);
            WriteFile(down_rate_limit_us_path6_7, "1500");
            WriteFile(up_rate_limit_us_path6_7, "1000");
        }
        for (int i = 0; i <= 7; ++i) {
            std::string policy = "policy" + std::to_string(i);
            readFrequencyTable(policy);
            setMaxFrequency(policy, 9); 
        }
    }
    void performance() {
        utils.log("性能模式已启用");
        schedhorizon();
        WriteFile(schedhorizon_path + "efficient_freq", "1700000");
        WriteFile(schedhorizon_path + "up_delay", "30");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1000");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "900");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */

        const std::string frequencies = "1400000 1700000 2000000 2500000";
        const std::string efficient_freq = "50 100 200 300";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);

            WriteFile(down_rate_limit_us_path, "1000");
            WriteFile(up_rate_limit_us_path, "900");
        }

        const std::string frequencies6_7 = "1200000 1800000 2500000";
        const std::string efficient_freq6_7 = "150 250 350";
        for (int i = 6; i <= 7; ++i) {
            const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            WriteFile(filePath6_7, frequencies);
            WriteFile(up_delayPath6_7, efficient_freq);
            WriteFile(down_rate_limit_us_path6_7, "1000");
            WriteFile(up_rate_limit_us_path6_7, "900");
        }
        for (int i = 0; i <= 7; ++i) {
            std::string policy = "policy" + std::to_string(i);
            readFrequencyTable(policy);
            setMaxFrequency(policy, 14); 
        }
    }

    void fast() {
        if (enableFeas) {
            Performance(); //Feas不基于任何调速器 他只负责进行调频
        EnableFeas();
        }else{
            utils.log("极速模式已切换 将更换Performance调速器 开启官调");
        Performance();
        }
    }

    void core_allocation() {
        if (coreAllocation) {
         utils.log("已开启核心绑定");
        WriteFile(background_cpu, "0-1");
        WriteFile(system_background_cpu, "0-2");
        WriteFile(foreground_cpu, "0-7");
        WriteFile(top_app, "0-7");
        }
    }

    void disable_qcomGpuBoost(){
        if (disableGpuBoost){
            std::string num_pwrlevels_path = "/sys/class/kgsl/kgsl-3d0/num_pwrlevels";
            std::ifstream file(num_pwrlevels_path);
        int num_pwrlevels;
            if (file >> num_pwrlevels) {
            int MIN_PWRLVL = num_pwrlevels - 1;
                std::string minPwrlvlStr = std::to_string(MIN_PWRLVL);
                utils.log("已关闭高通GPU Boost");
                WriteFile("/sys/class/kgsl/kgsl-3d0/default_pwrlevel", minPwrlvlStr);
                WriteFile("/sys/class/kgsl/kgsl-3d0/min_pwrlevel", minPwrlvlStr);
                WriteFile("/sys/class/kgsl/kgsl-3d0/max_pwrlevel", "0");
                WriteFile("/sys/class/kgsl/kgsl-3d0/thermal_pwrlevel", "0");   
                WriteFile("/sys/class/kgsl/kgsl-3d0/throttling", "0");
            }
        }
    }
};