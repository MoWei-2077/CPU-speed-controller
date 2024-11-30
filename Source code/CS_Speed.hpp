#pragma once
#include "utils.hpp"
#include "config.hpp"

class CS_Speed {
private:
    bool enableFeas;
    bool disableGpuBoost;
    bool coreAllocation;
    bool loadbalancing;
    bool DisableUFSclockgate;
    bool CFSscheduler;
    bool Cpuidle;
    bool UclampStrategy;
    bool DisableDetailedLog;
    bool Disable_AppLoadBalance;
    bool AffinitySetter;
    Utils utils;
    Config config;
    INIReader reader;
    std::vector<std::thread> threads;
    const std::string schedhorizon_path = "/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/";
    const std::string background_cpuset = "/dev/cpuset/background/cpus"; // 用户的后台应用
    const std::string system_background_cpuset = "/dev/cpuset/system-background/cpus"; // 系统的后台应用
    const std::string foreground_cpuset = "/dev/cpuset/foreground/cpus"; // 前台的应用
    const std::string top_app_cpuset = "/dev/cpuset/top-app/cpus"; // 顶层应用
    const std::string restricted_cpuset = "/dev/cpuset/restricted/cpus"; 
    const std::string top_app_cpuctl = "/dev/cpuctl/top-app/";
    const std::string cpuset_path = "/dev/cpuset/";
    const std::string foreground_cpuctl = "/dev/cpuctl/foreground/";
    const std::string Scheduler_path = "/proc/sys/kernel/";
    const std::string walt_path = "/proc/sys/walt/";
    const std::string CpuIdle_path = "/sys/devices/system/cpu/cpuidle/"; // CPU idle
    const std::string MTK_path = "/data/adb/modules/MW_CpuSpeedController/MTKPhone";
public:
    CS_Speed() : reader("/sdcard/Android/MW_CpuSpeedController/config.ini") {}
    void readAndParseConfig() {
        INIReader
            reader("/sdcard/Android/MW_CpuSpeedController/config.ini");

        if (reader.ParseError() < 0) {
            utils.log("警告:请检查您的配置文件是否存在");
            exit(0);
        }

        enableFeas = reader.GetBoolean("meta", "Enable_Feas", false);
        disableGpuBoost = reader.GetBoolean("meta", "Disable_qcom_GpuBoost", false);
        coreAllocation = reader.GetBoolean("meta", "Core_allocation", false);
        loadbalancing = reader.GetBoolean("meta", "Load_balancing", false);
        DisableUFSclockgate = reader.GetBoolean("meta", "Disable_UFS_clock_gate", false);
        Cpuidle = reader.GetBoolean("meta", "Adj_CpuIdle", false);
        CFSscheduler = reader.GetBoolean("meta", "CFS_Scheduler", false);
        UclampStrategy = reader.GetBoolean("meta", "New_Uclamp_Strategy", false);
        DisableDetailedLog = reader.GetBoolean("meta", "Disable_Detailed_Log", false);
        Disable_AppLoadBalance = reader.GetBoolean("meta", "Disable_App_Load_Balance", false);
        AffinitySetter = reader.GetBoolean("meta", "Affinity_Setter", false);
    }
    bool checkMTK_path(){
        return access(MTK_path.c_str(), F_OK) == 0;
    }

    void config_mode() {
        std::string line;
        std::ifstream file = config.Getconfig();
        while (std::getline(file, line)) {
            if (line == "powersave") {
                powersave();
            } else if (line == "balance") {
                balance();
            } else if (line == "performance") {
                performance();
            } else if (line == "fast") {
                fast();
            }
        }
    }
    void WriteFile(const std::string& filePath, const std::string& content) noexcept {
        int fd = open(filePath.c_str(), O_WRONLY | O_NONBLOCK);

        if (fd < 0) {
            chmod(filePath.c_str(), 0666);
            fd = open(filePath.c_str(), O_WRONLY | O_NONBLOCK); 
        }

        if (fd >= 0) {
            write(fd, content.data(), content.size());
            close(fd);
            chmod(filePath.c_str(), 0444);
        }
    }
    void CpuIdle(){
        if (!Cpuidle)
            return;
            
        // https://blog.csdn.net/youthcowboy/article/details/135348079 
        // 联发科:menu 高通:qcom-cpu-lpm

        if (checkMTK_path()){
            utils.log("CPUIdle governor已切换为menu");
            WriteFile(CpuIdle_path + "current_governor", "menu");
        } else {
            utils.log("CPUIdle governor已切换为qcom-cpu-lpm");
            WriteFile(CpuIdle_path + "current_governor", "qcom-cpu-lpm");
        }
    }
    void walt(){
        if (checkWalt()) {
            WriteFile("/sys/kernel/msm_performance/parameters/cpu_min_freq", "0:0 1:0 2:0 3:0 4:0 5:0 6:0 7:0");
            WriteFile("/sys/kernel/msm_performance/parameters/cpu_max_freq", "0:9999999 1:9999999 2:9999999 3:9999999 4:9999999 5:9999999 6:9999999 7:9999999");
            WriteFile(walt_path + "sched_busy_hyst_ns", "0");
            WriteFile(walt_path + "sched_group_upmigrate", "100");
            WriteFile(walt_path + "sched_asymcap_boost", "1");
            WriteFile(walt_path + "sched_force_lb_enable", "1");
            WriteFile(walt_path + "sched_boost", "0");
        for (int i = 0; i <= 7; ++i) {
                std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/walt/";
                WriteFile(cpuDir + "up_rate_limit_us", "1000");
                WriteFile(cpuDir + "down_rate_limit_us", "1000");
                WriteFile(cpuDir + "adaptive_high_freq", "0");
                WriteFile(cpuDir + "adaptive_low_freq", "0");
                WriteFile(cpuDir + "target_loads", "0");
                WriteFile(cpuDir + "target_load_thresh", "0");
                WriteFile(cpuDir + "target_load_shift", "3");
                WriteFile(cpuDir + "rtg_boost_freq", "0");
                WriteFile(cpuDir + "hispeed_freq", "0");
            }
        }
    }
    void reset(){
        for (int i = 0; i <= 7; ++i) {
            std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
            const std::string freq_limit = "/sys/devices/system/cpu/cpu" + std::to_string(i) +  "/cpufreq/scaling_min_freq_limit";
            const std::string freq_max = "/sys/devices/system/cpu/cpu" + std::to_string(i) +  "/cpufreq/scaling_max_freq";
            WriteFile(freq_limit, "0");
            WriteFile(freq_max, "2147483647");
            if (checkMTK_path()){
                WriteFile(cpuDir, "schedutil");       
            } else {
                WriteFile(cpuDir, "walt");
                walt();
            }
        }
        
        if (UclampStrategy){
            WriteFile(top_app_cpuctl + "cpu.uclamp.min.multiplier", "1");
            WriteFile(foreground_cpuctl + "cpu.uclamp.min.multiplier", "1");
        } else {
            WriteFile(top_app_cpuctl + "cpu.uclamp.min", "0");
            WriteFile(top_app_cpuctl + "cpu.uclamp.max", "max");
            WriteFile(foreground_cpuctl + "cpu.uclamp.min", "0");
            WriteFile(foreground_cpuctl + "cpu.uclamp.max", "max"); 
        }
        
        if (DisableUFSclockgate){
                WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "0"); 
            }else{
                WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "1"); 
        }
    }
    bool checkWalt(){
        const std::string walt_Path = "/sys/devices/system/cpu/cpufreq/policy0/walt";
        return access(walt_Path.c_str(), F_OK) == 0;
    }
    bool checkEAScheduler(){
        const std::string EAScheduler_path = "/proc/sys/kernel/sched_energy_aware";
        return access(EAScheduler_path.c_str(), F_OK) == 0;
    }
    bool checkCFS_scheduler(){
        const std::string CFScheduler_Path = "/proc/sys/kernel/sched_migration_cost_ns";   
        return access(CFScheduler_Path.c_str(), F_OK) == 0;
    }
    bool check_Spare_CFS_scheduler(){
        const std::string Spare_CFS_scheduler = "/sys/kernel/debug/sched/migration_cost_ns";   
        return access(Spare_CFS_scheduler.c_str(), F_OK) == 0;
    }
    void CFS_Scheduler(){
        if (!CFSscheduler){ // 使用卫语句简洁代码
            return;
        }
        if(checkCFS_scheduler()){
            WriteFile(Scheduler_path + "sched_migration_cost_ns", "5000000");
            WriteFile(Scheduler_path + "sched_min_granularity_ns", "12500000");
            utils.log("CFS调度器参数已调整完毕");
        } else if (check_Spare_CFS_scheduler()){
            WriteFile("/sys/kernel/debug/sched/migration_cost_ns", "5000000");
            WriteFile("/sys/kernel/debug/sched/wakeup_granularity_ns", "12500000");
            utils.log("CFS调度器参数已调整完毕");
        } else{
            utils.log("警告:您的设备不支持CFS调度器 请询问内核开发者解决问题");
        }
    }
    
    void EAS_Scheduler(){
        if (checkEAScheduler()){
            WriteFile(Scheduler_path + "sched_min_granularity_ns", "2000000"); // EAS 调度器中的最小调度粒度 调度器将任务划分为较小的时间片段进行调度 单位NS
            WriteFile(Scheduler_path + "sched_nr_migrate", "30");  // 用于控制任务在多个 CPU 核心之间迁移的次数
            WriteFile(Scheduler_path + "sched_wakeup_granularity_ns", "3200000"); // EAS 调度器可能会根据能效考虑来调整任务的唤醒时间 单位NS
            WriteFile(Scheduler_path + "sched_schedstats", "0"); // 禁用调度统计信息收集
            WriteFile(Scheduler_path + "sched_energy_aware", "1"); // 启用EAS调度器
            utils.log("EAS调度器已启用 参数已调整完毕");
        } else {
            utils.log("警告:您的设备不存在EAS调度器 请询问内核开发者解决问题");
        }
    }
    void schedhorizon() {
        for (int i = 0; i <= 7; ++i) {
            const std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
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
            WriteFile("/sys/module/perfmgr/parameters/perfmgr_enable", "0");
        }else if (checkMTKFeas()) {
            WriteFile("/sys/module/mtk_fpsgo/parameters/perfmgr_enable", "0");
        } 
    }
    void EnableFeas() {
        if (checkqcomFeas()) {
            utils.log("高通设备:Feas已启用"); // Feas已启用
            WriteFile("/sys/module/perfmgr/parameters/perfmgr_enable", "1");
        } else if (checkMTKFeas()) {
            utils.log("联发科设备:Feas已启用");
            WriteFile("/sys/module/mtk_fpsgo/parameters/perfmgr_enable", "1");
        }else {
            utils.log("警告:您的设备不支持Feas 请检查您设备是否拥有Perfmgr内核模块或没有FPSGO内核模块目前将不会调整任何模式请更换模式");        
        }
    }
    void powersave() {
        if (!DisableDetailedLog){
            utils.log("省电模式已启用");
        }
        schedhorizon();

        WriteFile(schedhorizon_path + "efficient_freq", "1500000");
        WriteFile(schedhorizon_path + "up_delay", "50");
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", "1000000");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */
        const std::string frequencies = "1200000 1500000 1800000 2300000";
        const std::string efficient_freq = "100 150 300 500";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq); 
            WriteFile(scaling_min_freq_limit_path, "1000000");
        }

        const std::string frequencies6_7 = "1200000 1800000 2300000";
        const std::string efficient_freq6_7 = "100 200 300";
        for (int i = 6; i <= 7; ++i) {
            const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            WriteFile(filePath6_7, frequencies6_7);
            WriteFile(up_delayPath6_7, efficient_freq6_7);
            WriteFile(scaling_min_freq_limit_path6_7, "900000");
        if (UclampStrategy){
            WriteFile(top_app_cpuctl + "cpu.uclamp.min.multiplier", "0.2");
            WriteFile(foreground_cpuctl + "cpu.uclamp.min.multiplier", "0.2");
        } else {
            WriteFile(top_app_cpuctl + "cpu.uclamp.min", "0");
            WriteFile(top_app_cpuctl + "cpu.uclamp.max", "80");
            WriteFile(foreground_cpuctl + "cpu.uclamp.min", "0");
            WriteFile(foreground_cpuctl + "cpu.uclamp.max", "70"); 
        }
            WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "1"); 
        }

        if (enableFeas){
            Feasdisable();
        }
    }
    void balance() {
        if (!DisableDetailedLog){
            utils.log("均衡模式已启用");
        }
        schedhorizon();

        WriteFile(schedhorizon_path + "efficient_freq", "1500000");
        WriteFile(schedhorizon_path + "up_delay", "40");
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", "1300000");

        const std::string frequencies = "1700000 2000000 2300000";
        const std::string efficient_freq = "50 100 100";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);
            WriteFile(scaling_min_freq_limit_path, "1400000");
        }

        const std::string frequencies6_7 = "1800000 2300000";
        const std::string efficient_freq6_7 = "50 100";
        for (int i = 6; i <= 7; ++i) {
            const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            WriteFile(filePath6_7, frequencies);
            WriteFile(up_delayPath6_7, efficient_freq);
            WriteFile(scaling_min_freq_limit_path6_7, "1600000");
        if (UclampStrategy){
            WriteFile(top_app_cpuctl + "cpu.uclamp.min.multiplier", "0.3");
            WriteFile(foreground_cpuctl + "cpu.uclamp.min.multiplier", "0.3");
        } else {
            WriteFile(top_app_cpuctl + "cpu.uclamp.min", "0");
            WriteFile(top_app_cpuctl + "cpu.uclamp.max", "80");
            WriteFile(foreground_cpuctl + "cpu.uclamp.min", "0");
            WriteFile(foreground_cpuctl + "cpu.uclamp.max", "70"); 
        }
            WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "1"); 
        }
        
        if (enableFeas){
            Feasdisable();
        }
    }
    void performance() {
        if (!DisableDetailedLog){
            utils.log("性能模式已启用");
        }
        schedhorizon();
        
        WriteFile(schedhorizon_path + "efficient_freq", "0"); 
        WriteFile(schedhorizon_path + "up_delay", "10");
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", "1500000");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */

        const std::string frequencies = "1700000 2000000 2500000";
        const std::string efficient_freq = "50 50 50";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);
            WriteFile(scaling_min_freq_limit_path, "1700000");
        }

        const std::string frequencies6_7 = "0";
        const std::string efficient_freq6_7 = "0";
        for (int i = 6; i <= 7; ++i) {
            const std::string up_delayPath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path6_7 = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            WriteFile(filePath6_7, frequencies);
            WriteFile(up_delayPath6_7, efficient_freq);
            WriteFile(scaling_min_freq_limit_path6_7, "1800000");
        if (UclampStrategy){
            WriteFile(top_app_cpuctl + "cpu.uclamp.min.multiplier", "0.5");
            WriteFile(foreground_cpuctl + "cpu.uclamp.min.multiplier", "0.5");
        } else {
            WriteFile(top_app_cpuctl + "cpu.uclamp.min", "0");
            WriteFile(top_app_cpuctl + "cpu.uclamp.max", "max");
            WriteFile(foreground_cpuctl + "cpu.uclamp.min", "0");
            WriteFile(foreground_cpuctl + "cpu.uclamp.max", "80"); 
        }
    }
        
        if (DisableUFSclockgate){
            WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "0"); 
        }else{
            WriteFile("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", "1"); 
        }
        
        if (enableFeas){
            Feasdisable();
        }
    }

    void fast() {
        if (!DisableDetailedLog){
            utils.log("极速模式已切换");
        }
        if (enableFeas) {
            EnableFeas();
        }
        reset();
    }
      
    void core_allocation() {
        if (!coreAllocation) {
            return;
        }
            utils.log("已开启核心绑定");
            WriteFile(background_cpuset, "0-2");
            WriteFile(system_background_cpuset, "0-2");
            WriteFile(restricted_cpuset, "0-5");
            WriteFile(foreground_cpuset, "0-3,4-7");
            WriteFile(top_app_cpuset, "0-7");
    }
    // 在大多数EAS平台上设置schedtune.boost > 0固定大核 
    // https://github.com/yc9559/uperf
     void disable_AppLoadBalance(){
        if (!Disable_AppLoadBalance) {
            return;
        }
        // ztc内核不支持 schedtune.boost 就关闭sched_ralex_domian_level得了
        WriteFile(cpuset_path + "foreground/sched_ralex_domian_level", "0");
        WriteFile(cpuset_path + "top-app/sched_ralex_domian_level", "0");
        WriteFile(cpuset_path + "background/sched_ralex_domian_level", "0");
        WriteFile(Scheduler_path +"sched_autogroup_enabled", "0");
     }

    void mount_cpuset(){
        mkdir("/dev/cpuset/top-app/MoWei", 0666);
        WriteFile("/dev/cpuset/top-app/MoWei/cpus", "7");
        WriteFile("/dev/cpuset/top-app/MoWei/mems", "0");
    }

    void affinitySetter(){
        if (!AffinitySetter){
            return;
        }
        mount_cpuset();
         
        // 下次这里改成遍历/proc/<pid>/cmdline 来获取pid 就不需要使用popen+shell来获取 大概可以提高一点点效率和性能
        const std::string system_server_pid = "pgrep -f system_server";
        const std::string system_server_pid_str = utils.exec(system_server_pid);
        const std::string surfaceflinger_pid = "pgrep -f surfaceflinger";
        const std::string surfaceflinger_pid_str = utils.exec(surfaceflinger_pid);
        
        std::string trimmed_pid_str = system_server_pid_str;
        std::string surfaceflinger_str = surfaceflinger_pid_str;
    
        trimmed_pid_str.erase(std::remove(trimmed_pid_str.begin(), trimmed_pid_str.end(), '\n'), trimmed_pid_str.end());
        surfaceflinger_str.erase(std::remove(surfaceflinger_str.begin(), surfaceflinger_str.end(), '\n'), surfaceflinger_str.end());
        
        std::string tids = utils.getTids(trimmed_pid_str, surfaceflinger_str);
        utils.FileWrite("/dev/cpuset/top-app/MoWei/cgroup.procs", trimmed_pid_str, surfaceflinger_str); // FileWrite函数需要传输两个参数才能进行写入操作
        WriteFile("/dev/cpuset/top-app/MoWei/tasks", tids.c_str()); // 使用WriteFile就可以了 不需要使用FileWrite 
        // 在同时写入一个文件时 如果需要写入两个参数 可以使用FileWrite写入器 他会更加高效 不需要像WriteFile打开两次文件 chmod 然后关闭文件 所以高效
    
        /*
          遍历system_server和surfaceflinger的Pid和Tid
          并将PID写入到/dev/cpuset/top-app/MoWei/cgroup.procs中
          再将Tid写入到/dev/cpuset/top-app/MoWei/tasks中
        */
       
    }
    void load_balancing() {
        if (!loadbalancing) {
            return;
        }
            utils.log("已开启负载均衡优化");
            WriteFile("/dev/cpuset/sched_relax_domain_level", "1");
            WriteFile("/dev/cpuset/system-background/sched_relax_domain_level", "1");
            WriteFile("/dev/cpuset/background/sched_relax_domain_level", "1");
            WriteFile("/dev/cpuset/foreground/sched_relax_domain_level", "1");
            WriteFile("/dev/cpuset/top-app/sched_relax_domain_level", "1");
    }

    void disable_qcomGpuBoost(){
        if (!disableGpuBoost){
            return;
        }
        const std::string num_pwrlevels_path = "/sys/class/kgsl/kgsl-3d0/num_pwrlevels";
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
            WriteFile("/sys/class/kgsl/kgsl-3d0/force_bus_on", "0");
            WriteFile("/sys/class/kgsl/kgsl-3d0/force_clk_on", "0");
            WriteFile("/sys/class/kgsl/kgsl-3d0/force_no_nap", "0");
            WriteFile("/sys/class/kgsl/kgsl-3d0/force_rail_on", "0");
            WriteFile("/sys/class/kgsl/kgsl-3d0/force_bus_on", "0");              
        }
    }
};