#include "CS_Speed.hpp"

int main() {
    std::string line;
    std::ifstream file;
    Utils utils;
    CS_Speed csspeed;
    Config config;
    
    utils.clear_log();// 先清理日志 否则无法正确抛出错误
    utils.Init();
    utils.Initconf();
    csspeed.schedhorizon();
    utils.Initschedhorizon();
    csspeed.readAndParseConfig();
    csspeed.disable_qcomGpuBoost();
    csspeed.core_allocation();
    csspeed.load_balancing();
    csspeed.EAScheduler();
    csspeed.config_mode();
    file = config.Getconfig();
    csspeed.config_mode();
    while (true) {
            utils.InotifyMain("/sdcard/Android/MW_CpuSpeedController/config.txt", IN_MODIFY); // 检测配置文件变化
            std::ifstream file = config.Getconfig();
        while (std::getline(file, line)) {
            if (line == "powersave") {
                csspeed.powersave();
            }
            else if (line == "balance") {
                 csspeed.balance();
            }
            else if (line == "performance") {
                csspeed.performance();
            }
            else if (line == "fast") {
                csspeed.fast();
            }
        }
    }
    return 0;
}