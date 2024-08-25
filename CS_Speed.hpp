#include "utils.hpp"
#include "config.hpp"

class cs_speed {
private:
    const std::string schedhorizon_path = "/sys/devices/system/cpu/cpufreq/policy0/schedhorizon/"; 
    const std::string cpu_uclamp_min = "/dev/cpuctl/top-app/cpu.uclamp.min";
    const std::string cpu_uclamp_max = "/dev/cpuctl/top-app/cpu.uclamp.max";
    const std::string foreground_cpu_uclamp_min = "/dev/cpuctl/foreground/cpu.uclamp.min";
    const std::string foreground_cpu_uclamp_max = "/dev/cpuctl/foreground/cpu.uclamp.max";

public:
    std::vector<int> read_cpu_freqs(const std::string& path) {
        std::ifstream file(path + "/scaling_available_frequencies");
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading frequencies." << std::endl;
            return {};
        }

        std::vector<int> freqs;
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            int freq;
            while (iss >> freq) {
                freqs.push_back(freq);
            }
        }

        std::sort(freqs.begin(), freqs.end());
        return freqs;
    }

    std::map<int, int> index_freq_map(const std::vector<int>& freqs) {
        std::map<int, int> freq_index_map;
        for (size_t i = 0; i < freqs.size(); ++i) {
            freq_index_map[i + 1] = freqs[i];
        }
        return freq_index_map;
    }

    void APPfrequencyupgrade() {
        performance();
        log("检测到App已切换 正在切换模式");
        sleep (3);
        log("冷启动完成 正在恢复省电模式");
        powersave();
    }

    void powersave() {
        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表

        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq = freq_index_map[6]; 
        int min_freq = freq_index_map[3];  
        int min_freq_limit = freq_index_map[1];

        WriteFile(schedhorizon_path + "efficient_freq", "1700000");
        WriteFile(schedhorizon_path + "up_delay", "50");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1500");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "1000");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", std::to_string(min_freq_limit)); // 写入最低频率
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq", std::to_string(min_freq)); // 写入最低频率
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", std::to_string(max_freq)); // 写入最大频率

        const std::string frequencies = "1400000 1700000 2000000 2500000";
        const std::string efficient_freq = "200 200 300 500";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            const std::string scaling_min_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq";
            const std::string scaling_max_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_max_freq";

        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表
        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq1_5 = freq_index_map[6]; 
        int min_freq1_5 = freq_index_map[2];  //  适当放宽频率
        int min_freq_limit1_5 = freq_index_map[1];
            WriteFile(scaling_max_freq, std::to_string(max_freq1_5));
            WriteFile(scaling_min_freq, std::to_string(min_freq1_5));
            WriteFile(scaling_min_freq_limit_path, std::to_string(min_freq_limit1_5));
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);
            
            WriteFile(down_rate_limit_us_path, "2000");
            WriteFile(up_rate_limit_us_path, "6000");
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

        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表
        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq6_7 = freq_index_map[5]; 
        int min_freq6_7 = freq_index_map[1];  
        int min_freq_limit6_7 = freq_index_map[1];
            WriteFile(scaling_max_freq, std::to_string(max_freq6_7));
            WriteFile(scaling_min_freq, std::to_string(min_freq6_7));
            WriteFile(scaling_min_freq_limit_path, std::to_string(min_freq_limit6_7));
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);
            WriteFile(down_rate_limit_us_path6_7, "2000");
            WriteFile(up_rate_limit_us_path6_7, "6000");
        }
    }
    void balance(){
        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表

        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq = freq_index_map[6];
        int min_freq = freq_index_map[3];
        int min_freq_limit = freq_index_map[1];

        WriteFile(schedhorizon_path + "efficient_freq", "1700000");
        WriteFile(schedhorizon_path + "up_delay", "50");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1500");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "1000");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", std::to_string(min_freq_limit)); // 写入最低频率
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq", std::to_string(min_freq)); // 写入最低频率
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", std::to_string(max_freq)); // 写入最大频率

        const std::string frequencies = "1400000 1700000 2000000 2500000";
        const std::string efficient_freq = "200 200 300 500";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            const std::string scaling_min_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq";
            const std::string scaling_max_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_max_freq";

        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表
        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq1_5 = freq_index_map[6];
        int min_freq1_5 = freq_index_map[2];  //  适当放宽频率
        int min_freq_limit1_5 = freq_index_map[1];
            WriteFile(scaling_max_freq, std::to_string(max_freq1_5));
            WriteFile(scaling_min_freq, std::to_string(min_freq1_5));
            WriteFile(scaling_min_freq_limit_path, std::to_string(min_freq_limit1_5));
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);

            WriteFile(down_rate_limit_us_path, "2000");
            WriteFile(up_rate_limit_us_path, "6000");
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

        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表
        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq6_7 = freq_index_map[5];
        int min_freq6_7 = freq_index_map[1];
        int min_freq_limit6_7 = freq_index_map[1];
            WriteFile(scaling_max_freq, std::to_string(max_freq6_7));
            WriteFile(scaling_min_freq, std::to_string(min_freq6_7));
            WriteFile(scaling_min_freq_limit_path, std::to_string(min_freq_limit6_7));
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);
            WriteFile(down_rate_limit_us_path6_7, "2000");
            WriteFile(up_rate_limit_us_path6_7, "6000");
        }
    }
    void performance(){
        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表

        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq = freq_index_map[6];
        int min_freq = freq_index_map[3];
        int min_freq_limit = freq_index_map[1];

        WriteFile(schedhorizon_path + "efficient_freq", "1700000");
        WriteFile(schedhorizon_path + "up_delay", "50");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1500");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "1000");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", std::to_string(min_freq_limit)); // 写入最低频率
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq", std::to_string(min_freq)); // 写入最低频率
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", std::to_string(max_freq)); // 写入最大频率

        const std::string frequencies = "1400000 1700000 2000000 2500000";
        const std::string efficient_freq = "200 200 300 500";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            const std::string scaling_min_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq";
            const std::string scaling_max_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_max_freq";

        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表
        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq1_5 = freq_index_map[6];
        int min_freq1_5 = freq_index_map[2];  //  适当放宽频率
        int min_freq_limit1_5 = freq_index_map[1];
            WriteFile(scaling_max_freq, std::to_string(max_freq1_5));
            WriteFile(scaling_min_freq, std::to_string(min_freq1_5));
            WriteFile(scaling_min_freq_limit_path, std::to_string(min_freq_limit1_5));
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);

            WriteFile(down_rate_limit_us_path, "2000");
            WriteFile(up_rate_limit_us_path, "6000");
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

        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表
        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq6_7 = freq_index_map[5];
        int min_freq6_7 = freq_index_map[1];
        int min_freq_limit6_7 = freq_index_map[1];
            WriteFile(scaling_max_freq, std::to_string(max_freq6_7));
            WriteFile(scaling_min_freq, std::to_string(min_freq6_7));
            WriteFile(scaling_min_freq_limit_path, std::to_string(min_freq_limit6_7));
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);
            WriteFile(down_rate_limit_us_path6_7, "2000");
            WriteFile(up_rate_limit_us_path6_7, "6000");
        }
    }

        void fast(){
        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表

        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq = freq_index_map[6];
        int min_freq = freq_index_map[3];
        int min_freq_limit = freq_index_map[1];

        WriteFile(schedhorizon_path + "efficient_freq", "1700000");
        WriteFile(schedhorizon_path + "up_delay", "50");
        WriteFile(schedhorizon_path + "down_rate_limit_us", "1500");
        WriteFile(schedhorizon_path + "up_rate_limit_us", "1000");

        /*
          当系统试图将 CPU 核心的频率设置得比 scaling_min_freq_limit 更低时，
          实际设置的 scaling_min_freq 值会等于 scaling_min_freq_limit
        */
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq_limit", std::to_string(min_freq_limit)); // 写入最低频率
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq", std::to_string(min_freq)); // 写入最低频率
        WriteFile("/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq", std::to_string(max_freq)); // 写入最大频率

        const std::string frequencies = "1400000 1700000 2000000 2500000";
        const std::string efficient_freq = "200 200 300 500";
        for (int i = 1; i <= 5; ++i) {
            const std::string up_delayPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/up_delay";
            const std::string filePath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/schedhorizon/efficient_freq";
            const std::string scaling_min_freq_limit_path = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq_limit";
            const std::string down_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/down_rate_limit_us";
            const std::string up_rate_limit_us_path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/schedhorizon/up_rate_limit_us";
            const std::string scaling_min_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_min_freq";
            const std::string scaling_max_freq = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_max_freq";

        std::vector<int> freqs = read_cpu_freqs("/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"); // 读取频率表
        std::map<int, int> freq_index_map = index_freq_map(freqs); // 将频率表中的频率值与索引关联起来
        int max_freq1_5 = freq_index_map[6];
        int min_freq1_5 = freq_index_map[2];  //  适当放宽频率
        int min_freq_limit1_5 = freq_index_map[1];
            WriteFile(scaling_max_freq, std::to_string(max_freq1_5));
            WriteFile(scaling_min_freq, std::to_string(min_freq1_5));
            WriteFile(scaling_min_freq_limit_path, std::to_string(min_freq_limit1_5));
            WriteFile(filePath, frequencies);
            WriteFile(up_delayPath, efficient_freq);

            WriteFile(down_rate_limit_us_path, "2000");
            WriteFile(up_rate_limit_us_path, "6000");
        }
    }
    void schedhorizon(){
        for (int i = 0; i <= 7; ++i) {
            std::string cpuDir = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(i) + "/scaling_governor";
            WriteFile(cpuDir, "schedhorizon");
        }
    }

    bool checkschedhorizon(){
        const std::string schedhorizon_path = "/sys/devices/system/cpu/cpufreq/policy0/schedhorizon";
        return access(schedhorizon_path.c_str(), F_OK) == 0;
    }

    void Initschedhorizon(){
        if (checkschedhorizon()) {
            Log("您的设备支持CS调度");
        } else {
            Log("警告:您的设备不支持CS调度 CS调度进程已退出");
            exit(1);
        }
    }
    void core_allocation() {
        WriteFile(background_cpu, "0-1");
        WriteFile(system_background_cpu, "0-2");
        WriteFile(foreground_cpu, "0-6");
        WriteFile(top_app, "0-7");
    }

    void disable_qcomGpuBoost(){
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

    void Restore_Official(){
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
    bool checkFeas(){
        const std::string Feas_path = "/sys/module/perfmgr/parameters/perfmgr_enable";
        return access(Feas_path.c_str(), F_OK) == 0;
    }
    void Feasdisable(){
        WriteFile("/sys/module/perfmgr/parameters/perfmgr_enable", "0");
    }
    void enableFeas(){
       if (checkFeas()){
          Log("Feas已启用"); // Feas已启用
          WriteFile("/sys/module/perfmgr/parameters/perfmgr_enable", "1");
       }else{
          Log("警告:您的设备不支持Feas 请检查您设备是否拥有Perfmgr模块 将为您开启极速模式");
          fast();
       }
    }
};