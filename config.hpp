#include <iostream>
class Config {
private:
    const std::string confpath = "/sdcard/Android/MW_CpuSpeedController/config.txt";
    const std::string configpath = "/sdcard/Android/MW_CpuSpeedController/conf.ini";

public:
    std::ifstream Getconfig() {
        std::ifstream file(confpath);
        return file;
    }
};