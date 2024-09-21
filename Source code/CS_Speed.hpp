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
    bool loadbalancing;
    bool DisableUFSclockgate;
    bool refresh;
    Utils utils;
    Config config;
    INIReader reader;
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
        loadbalancing = reader.GetBoolean("meta", "Load_balancing", false);
        DisableUFSclockgate = reader.GetBoolean("meta", "Disable_UFS_clock_gate", false);
        refresh = reader.GetBoolean("meta", "Refresh", false);
    }
    std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

std::string getProcessName(const std::string& statusContent) {
    size_t start = statusContent.find("Name:\t");
    if (start != std::string::npos) {
        start += 6; 
        size_t end = statusContent.find('\n', start);
        return statusContent.substr(start, end - start);
    }
    return "";
}
 
    void config_mode(){
        std::string line;
        std::ifstream file = config.Getconfig();
        while (std::getline(file, line)) {
            if (line == "powersave") {
                powersave();
            }
            else if (line == "balance") {
                balance();
            }
            else if (line == "performance") {
                performance();
            }
            else if (line == "fast") {
                fast();
            }
        }
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
    void Performance(){
         for (int i = 0; i <= 7; ++i) {
            std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
            WriteFile(cpuDir, "performance");
        }
      WriteFile("/dev/cpuctl/top-app/cpu.uclamp.min", "0");
      WriteFile("/dev/cpuctl/top-app/cpu.uclamp.max", "max");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "10");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "80");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "10");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "30"); 
        if (DisableUFSclockgate){
            WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "0"); 
            }else{
            WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "1"); 
        }
    }
    bool checkEAScheduler(){
        const std::string EAScheduler_path = "/proc/sys/kernel/sched_energy_aware";
        return access(EAScheduler_path.c_str(), F_OK) == 0;
    }
    void EAScheduler(){
        if (checkEAScheduler()){
            const std::string EAS_Path = "/proc/sys/kernel/";
            WriteFile(EAS_Path + "sched_min_granularity_ns", "2000000"); // EAS 调度器中的最小调度粒度 调度器将任务划分为较小的时间片段进行调度 单位NS
            WriteFile(EAS_Path + "sched_nr_migrate", "30");  // 用于控制任务在多个 CPU 核心之间迁移的次数
            WriteFile(EAS_Path + "sched_wakeup_granularity_ns", "3000000"); // EAS 调度器可能会根据能效考虑来调整任务的唤醒时间 单位NS
            WriteFile(EAS_Path + "sched_schedstats", "0"); // 禁用调度统计信息收集
            WriteFile(EAS_Path + "sched_energy_aware", "1"); // 启用EAS调度器
            utils.log("EAS调度器已启用 参数已调整完毕");
        }else{
            utils.log("警告:您的设备不存在EAS调度器 请询问内核开发者解决问题");
        }
    }
    void schedhorizon() {
        for (int i = 0; i <= 7; ++i) {
            std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
            WriteFile(cpuDir, "schedhorizon");
        }
    }
    bool checkqcomFeas() {
        const std::string QcomFeas_path = "/sys/module/perfmgr/parameters/perfmgr_enable";
        return access(QcomFeas_path.c_str(), F_OK) == 0;
    }
    bool checkMTKFeas() {
        const std::string MTKFeas_path = "/sys/module/mtk_fpsgo/parameters/perfmgr_enable";
        return access(MTKFeas_path.c_str(), F_OK) == 0;
    }
   
    void Feasdisable() { // Feas可能会导致日常卡顿 所以在日常中需要进行关闭Feas
        if (checkqcomFeas()) {
            utils.log("Feas已关闭"); 
            WriteFile("/sys/module/perfmgr/parameters/perfmgr_enable", "0");
        }
        else if (checkMTKFeas()) {
            utils.log("Feas已关闭");
            WriteFile("/sys/module/mtk_fpsgo/parameters/perfmgr_enable", "0");
        } 
    }
    void EnableFeas() {
        if (checkqcomFeas()) {
            utils.log("Feas已启用"); // Feas已启用
            WriteFile("/sys/module/perfmgr/parameters/perfmgr_enable", "1");
        }
        else if (checkMTKFeas()) {
            utils.log("Feas已启用");
            WriteFile("/sys/module/mtk_fpsgo/parameters/perfmgr_enable", "1");
        }else{
            utils.log("警告:您的设备不支持Feas 请检查您设备是否拥有Perfmgr模块 由于您的设备没有Perfmgr模块目前将不会调整任何模式请更换模式");        
        }
    }
    void powersave() {
        utils.log("省电模式已启用");
        schedhorizon();

        WriteFile(schedhorizon_path + "efficient_freq", "1700000");
        WriteFile(schedhorizon_path + "up_delay", "60");
        WriteFile(schedhorizon_path + "scaling_min_freq_limit", "900000");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "500");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "3000");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */
        const std::string frequencies = "1400000 1700000 2000000 2500000";
        const std::string efficient_freq = "200 200 300 500";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq); 
            WriteFile(scaling_min_freq_limit_path, "900000");
            WriteFile(down_rate_limit_us_path, "500");
            WriteFile(up_rate_limit_us_path, "3000");
        }

        const std::string frequencies6_7 = "1200000 1800000 2500000";
        const std::string efficient_freq6_7 = "150 500 500";
        for (int i = 6; i <= 7; ++i) {
            const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            WriteFile(filePath6_7, frequencies6_7);
            WriteFile(up_delayPath6_7, efficient_freq6_7);
            WriteFile(scaling_min_freq_limit_path6_7, "900000");
            WriteFile(down_rate_limit_us_path6_7, "500");
            WriteFile(up_rate_limit_us_path6_7, "3000");
        }
      WriteFile("/dev/cpuctl/top-app/cpu.uclamp.min", "0");
      WriteFile("/dev/cpuctl/top-app/cpu.uclamp.max", "80");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "0");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "70");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "0");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "30");
      WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "1"); 
      Feasdisable();
    }
    void balance() {
        utils.log("均衡模式已启用");
        schedhorizon();

        WriteFile(schedhorizon_path + "efficient_freq", "1700000");
        WriteFile(schedhorizon_path + "up_delay", "50");
        WriteFile(schedhorizon_path + "scaling_min_freq_limit", "1400000");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1500");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "1000");

        const std::string frequencies = "1400000 1700000 2000000 2500000";
        const std::string efficient_freq = "50 100 100 150";
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
            WriteFile(scaling_min_freq_limit_path, "1200000");
            WriteFile(down_rate_limit_us_path, "1500");
            WriteFile(up_rate_limit_us_path, "1000");
        }

        const std::string frequencies6_7 = "1200000 1800000 2500000";
        const std::string efficient_freq6_7 = "50 150 150";
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
            WriteFile(scaling_min_freq_limit_path6_7, "1200000");
            WriteFile(down_rate_limit_us_path6_7, "1500");
            WriteFile(up_rate_limit_us_path6_7, "1000");
        }
      WriteFile("/dev/cpuctl/top-app/cpu.uclamp.min", "10");
      WriteFile("/dev/cpuctl/top-app/cpu.uclamp.max", "max");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "0");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "80");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "0");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "30"); 
      WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "1"); 
      Feasdisable();
    }
    void performance() {
        utils.log("性能模式已启用");
        schedhorizon();
        WriteFile(schedhorizon_path + "efficient_freq", "0"); // 不进行频率限制
        WriteFile(schedhorizon_path + "up_delay", "10");
        WriteFile(schedhorizon_path + "scaling_min_freq_limit", "1700000");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1000");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "800");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */

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
            WriteFile(scaling_min_freq_limit_path + "scaling_min_freq_limit", "1800000");
            WriteFile(down_rate_limit_us_path, "1000");
            WriteFile(up_rate_limit_us_path, "800");
        }

        const std::string frequencies6_7 = "1200000 1800000 2500000";
        const std::string efficient_freq6_7 = "50 150 150";
        for (int i = 6; i <= 7; ++i) {
            const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path6_7 = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            WriteFile(filePath6_7, frequencies);
            WriteFile(up_delayPath6_7, efficient_freq);
            WriteFile(scaling_min_freq_limit_path6_7 + "scaling_min_freq_limit", "1800000");
            WriteFile(down_rate_limit_us_path6_7, "1000");
            WriteFile(up_rate_limit_us_path6_7, "800");
        }
      WriteFile("/dev/cpuctl/top-app/cpu.uclamp.min", "10");
      WriteFile("/dev/cpuctl/top-app/cpu.uclamp.max", "max");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "10");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "80");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.min", "0");
      WriteFile("/dev/cpuctl/foreground/cpu.uclamp.max", "30"); 
      Feasdisable();
        if (DisableUFSclockgate){
            WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "0"); 
            }else{
            WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "1"); 
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
    void load_balancing() {
        if (loadbalancing) {
            utils.log("已开启负载均衡优化");
            WriteFile("/dev/cpuset/sched_relax_domain_level", "1");
            WriteFile("/dev/cpuset/system-background/sched_relax_domain_level", "1");
            WriteFile("/dev/cpuset/background/sched_relax_domain_level", "1");
            WriteFile("/dev/cpuset/foreground/sched_relax_domain_level", "1");
            WriteFile("/dev/cpuset/top-app/sched_relax_domain_level", "1");
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