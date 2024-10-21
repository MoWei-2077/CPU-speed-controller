#pragma once
#include "CS_Speed.hpp"

int main() {
    std::string line;

    Utils utils;
    CS_Speed csspeed;
    Config config;
    Controller controller;
    
    utils.clear_log();// 先清理日志 否则无法正确抛出错误
    usleep(500 * 1000); // 先等待一会 部分设备这个时候还在insmod schedhorizon.ko中
    utils.Init();
    csspeed.schedhorizon();
    usleep(200 * 1000); // 这里缓慢一点 等待schedhorizon.ko被insmod完成
    utils.Initschedhorizon();
    csspeed.readAndParseConfig();
    csspeed.disable_qcomGpuBoost();
    csspeed.core_allocation();
    csspeed.load_balancing();
    csspeed.EAS_Scheduler();
    csspeed.CFS_Scheduler();    
    csspeed.Touchboost();
    csspeed.config_mode();

    pid_t fPid = fork();
    if (fPid == -1) { 

        utils.log("fork控制进程失败，请使用scene接管");
        
    }

    if (fPid == 0) { // 控制器进程
        utils.log("fork控制进程成功");
        controller.bootFunction();
        exit(0); 
    }

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