# CPU调速器 （CS调度)
[![C++](https://img.shields.io/badge/language-C++-%23f34b7d.svg?style=plastic)](https://en.wikipedia.org/wiki/C++)
[![Android](https://img.shields.io/badge/platform-Android-0078d7.svg?style=plastic)](https://en.wikipedia.org/wiki/Android_(operating_system)) 
[![AArch64](https://img.shields.io/badge/arch-AArch64-red.svg?style=plastic)](https://en.wikipedia.org/wiki/AArch64)
[![Android Support12-14](https://img.shields.io/badge/Android%2012~14-Support-green)](https://img.shields.io/badge/Android%2012~14-Support-green)
#### 介绍
1.该项目是基于C++进行编写的一款智能CPU调度 <br>
2.该调度依赖于schedhorizon调速器和EAS调度器 <br>
3.该调度为ZTC内核专属调度 当然别的内核也能用 但效果不如ztc内核

#### 工作条件
1.目前该调度适用于Android12-14 <br>
2.内核要求:5.10-5.15的GKI内核 PS:不推荐潘多拉和魔理沙内核进行配合使用

## 安装
1.下载后通过Magisk Manager刷入，Magisk版本不低于20.4 <br>
#### 修改启动时的默认模式
1.打开/sdcard/Android/MW_CpuSpeedController/config.txt <br>
2.可选的挡位有powersave balance performance fast四个挡位 <br>
2.重启后查看/sdcard/Android/MW_CpuSpeedController/log.txt检查CS调度是否正常自启动

### 性能模式切换
powersave 省电模式 保证基本流畅的同时尽可能降低功耗 推荐日常使用 <br>
balance均衡模式，比原厂略流畅的同时略省电 推荐日常使用 <br>
performance性能模式，保证费电的同时多一点流畅度 推荐游戏使用 <br>
fast极速模式，默认官调 如果开启了Feas开关将会启用Feas

## 常见问题
Q：是否对待机功耗有负面影响？<br>
A：CS调度的实现做了不少低功耗的优化，得益于C++语言所以自身运行的功耗开销很低。 <br>
Q：为什么使用了CS调度功耗还是好高？ <br>
A：SOC的AP部分功耗主要取决于计算量以及使用的频点。CS调度只能控制性能释放，改进频率的方式从而降低功耗，如果后台APP的计算量很大是无法得到显著的续航延长的。这一问题可以通过Scene工具箱的进程管理器来定位。 <br>
Q: 为什么我的游戏大核负载异常？ <br>
A: 开启负载均衡。 PS:旧版禁止反馈 <br>
Q: 什么时候更新XXXX版本？ <br>
A: 请将需要更新的内容,发送至我的邮箱,邮箱:mowei2077@gmail.com😋。 <br>
Q: 是否需要调整EAS调度器？ <br>
A: CS调度在8.0版本会自动调整EAS调度器的参数,无需用户自行调整。 <br>
Q: 是否需要调整DDR LLCC L3 DDRQOS？ <br>
A: CS调度在9.1版本中锁定了DDR LLCC L3 DDRQOS值为9999000000,所以无需用户自行调整。 <br>
Q: 我该如何确保我的设备拥有Perfmgr内核模块？ <br>
A: 开启CS调度的Feas开关 切换为极速模式 CS调度会自动识别是否拥有Perfmgr内核模块 如果有将开启Feas 如果没有将会抛出错误在日志中 PS:如果需要Feas推荐刷入VK内核 目前CS调度已接入VK内核的Feas功能。
## 详细介绍 
该模块使用的调速器是schedhorizon performance<br>
所以在部分场景中得益于schedhorizon调速器会比Powersave调速器拥有更快的响应速度、性能稳定性或资源利用率。适当的调度策略可以确保系统在不同负载下的表现良好 <br>
在游戏场景中开启schedutil调速器 平衡功耗的同时保证流畅 <br>
废话不多说进入本篇的重点 配置文档

### 自定义配置文件
### 元信息

```ini
   [meta]
   name = "CS调度配置文件V3.0"
   author = "CoolApk@MoWei"
   Enable_Feas = false
   Disable_qcom_GpuBoost = false
   Core_allocation = false
   Load_balancing = false
   Disable_UFS_clock_gate = false
   TouchBoost = false
```
| 字段名   | 数据类型 | 描述                                           |
| -------- | -------- | ---------------------------------------------- |
| name     | string   | 配置文件的名称和版本                                 |
| author   | string   | 配置文件的作者信息                             |
| Working_mode | string   | CS调度的工作调速器，目前该字段不起任何作用 |
| Enable_Feas | bool   | 开启此功能后再开启极速模式就会恢复schedutil调速器再开启Feas |
| Disable_qcom_GpuBoost | bool   | 开启后将会关闭高通的GPUBoost 防止GPUBoost乱升频 |
| Core_allocation | bool   | 核心绑定 开启后将会调整应用的CPUSET与绑定线程的CPUSET不产生冲突 例如:A-SOUL和Scene的核心绑定 |
| Load_balancing | bool   | 负载均衡 |
| Disable_UFS_clock_gate | bool   | 开启后将在性能模式和极速模式关闭UFS时钟门 关闭后将会减少I/O资源消耗 提高耗电和性能 PS:省电模式和均衡模式因为功耗影响默认开启UFS时钟门 |
| TouchBoost | bool   | 开启后将启用内核的触摸升频 |
| CFS_Scheduler | bool   | 开启后将调整CFS调度器的参数 PS:该功能目前还在测试中 |
```
在CS启动时会读取`switchInode`对应路径的文件获取默认性能模式,在日志以如下方式体现：  
2024-08-11 11:22:37] 更换模式为性能模式
```
`switchInode`对应路径的文件，监听新模式名称的写入完成模式切换：  
```shell
echo "powersave" > /sdcard/Android/MW_CpuSpeedController/log.txt
```
在日志以如下方式体现：
2024-08-11 11:22:37] 更换模式为省电模式

# 致谢 （排名不分前后）
感谢以下用户对本项目的帮助：  
- [CoolAPK@ztc1997](https://github.com/ztc1997) <br>
- [CoolAPK@XShe](https://github.com/xsheeee) <br>
- [CoolAPK@Timeline](https://github.com/nep-Timeline) <br>
- QQ@长虹久奕
- CoolAPK@RalseiNEO
# 使用的开源项目
[作者:wme7 项目:INIreader](https://github.com/wme7/INIreader) <br>
感谢所有用户的测试反馈 这将推进CPU调速器(CS调度)的开发
### 该文档更新于:2024/10/06 18:14
