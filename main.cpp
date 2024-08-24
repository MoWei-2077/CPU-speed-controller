#include "utils.hpp"

int main(){
	clear_log(); // 清空日志
	Init(); // 检测进程是否多开并进行初始化
	schedhorizon();
	Initschedhorizon();
	Log("调度已进入工作");
	usleep(1000 * 100); // 100us
	Getjson();
    core_allocation();
	Getconfig(config_path); // 先进行获取配置文件内容再进行调整调度
while (true){
	sleep(1); // 上sleep 1s 防止cpu瞬时占用过高
	InotifyMain("/sdcard/Android/MW_CpuSpeedController/config.txt", IN_MODIFY); // 检测配置文件变化
	Getconfig(config_path); // 获取配置文件内容再进行调整调度
	}
}