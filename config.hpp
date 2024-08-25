#pragma once
#include "utils.hpp"
#include "CS_Speed.hpp"
class Config{
private:
    const std::string confpath = "/sdcard/Android/MW_CpuSpeedController/config.txt";
    const std::string confgpath = "/sdcard/Android/MW_CpuSpeedController/conf.ini";
public:

    void Getconfig() {
        std::ifstream file(confpath);
        std::string line;
        std::map<std::string, std::map<std::string, std::string>> iniData = readIniFile(configpath);
        while (std::getline(file, line)) { 
            if (line == "powersave") {
                CS_speed.powersave();
            } else if (line == "balance") {
                CS_speed.balance();
            } else if (line == "performance") {
                CS_speed.performance();
            } else if (line == "fast") {
            if (iniData["meta"]["Enable_Feas"] == "true") {
                CS_speed.enableFeas();
               }else{
                  fast();
               }
            } else if (line == "super_powersave") {
                CS_speed.super_powersave();
            }
        }
    }
};