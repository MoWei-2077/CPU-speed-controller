#include "CS_Speed.hpp"


int main() {
    std::string line;
    std::ifstream file;
    Utils utils;
    CS_Speed csspeed;
    Config config; // 使用正确的类名 Config

    utils.Init();
    utils.clear_log();
    csspeed.schedhorizon();
    utils.Initschedhorizon();
    csspeed.readAndParseConfig();
    csspeed.disable_qcomGpuBoost();
    csspeed.core_allocation();
    csspeed.AppFrequencyUpgrade();
    file = config.Getconfig();
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
            //if (iniData["meta"]["Enable_Feas"] == "true") {
            //    csSpeedInstance.enableFeas();
            //} else {
            csspeed.fast();
            //}
        }
        //else if (line == "super_powersave") {
        //    csspeed.super_powersave();
        //}
    }

    while (true) {
        sleep(1);
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
                //}
            }
        }
    }

    return 0;
}