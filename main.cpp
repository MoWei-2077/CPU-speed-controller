#include "utils.hpp"
#include "CS_Speed.hpp"
#include "config.hpp"

int main(){
   Utils utils;
   CS_Speed cs_speed;
   config Config;
   utils.Init();
   utils.clear_log();
   cs_speed.schedhorizon();
   cs_speed.Initschedhorizon();
   //Getjson();
   Config.Getconfig();
   While (true){
      sleep(1);
      utils.InotifyMain("/sdcard/Android/MW_CpuSpeedController/config.txt", IN_MODIFY); // 检测配置文件变化
      Config.Getconfig();
   }
   std::thread triggerTaskThread(&Utils::cpuSetTriggerTask, utils);
   // 让线程独立运行
   triggerTaskThread.detach();
}
