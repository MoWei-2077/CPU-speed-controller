#include "CS_Speed.hpp"
int main() {

    Utils utils;
    CS_Speed csspeed;
    Config config;
    std::vector<std::thread> threads;
    utils.clear_log();// 先清理日志 否则无法正确抛出错误
    usleep(500 * 1000); // 先等待一会 部分设备这个时候还在insmod schedhorizon.ko中
    utils.Init();
    csspeed.schedhorizon();
    usleep(200 * 1000); // 这里缓慢一点 等待schedhorizon.ko被insmod完成
    utils.Initschedhorizon();
    csspeed.readAndParseConfig();
    csspeed.AddFunction(); 
    csspeed.config_mode();
    csspeed.Thread_Conf();
    while (true) {
        utils.InotifyMain("/sdcard/Android/MW_CpuSpeedController/config.txt", IN_MODIFY); // 检测配置文件变化
        csspeed.config_mode();
    }
    return 0;
}