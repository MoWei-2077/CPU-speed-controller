#include "CS_Speed.hpp"

int main() {
    std::string line;

    Utils utils;
    CS_Speed csspeed;
    Config config;
    
    utils.clear_log();// 先清理日志 否则无法正确抛出错误
    utils.Init();
    csspeed.schedhorizon();
    utils.Initschedhorizon();
    csspeed.readAndParseConfig();
    csspeed.disable_qcomGpuBoost();
    csspeed.core_allocation();
    csspeed.load_balancing();
    csspeed.EAS_Scheduler();
    csspeed.CFS_Scheduler();
    csspeed.config_mode();
    csspeed.Touchboost();
    while (true) {
            utils.InotifyMain("/sdcard/Android/MW_CpuSpeedController/config.txt", IN_MODIFY); // 检测配置文件变化
            std::ifstream file = config.Getconfig();
        while (std::getline(file, line)) {
            if (line == "powersave") {
                csspeed.powersave();
            }
            if (line == "balance") {
                 csspeed.balance();
            }
            if (line == "performance") {
                csspeed.performance();
            }
            if (line == "fast") {
                csspeed.fast();
            }
        }
    }
    return 0;
}