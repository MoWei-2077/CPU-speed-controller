# CPU调速器 （CS调度）

[![C++](https://img.shields.io/badge/language-C++-%23f34b7d.svg?style=plastic)](https://en.wikipedia.org/wiki/C++)
[![Android](https://img.shields.io/badge/platform-Android-0078d7.svg?style=plastic)](https://en.wikipedia.org/wiki/Android_(operating_system)) 
[![AArch64](https://img.shields.io/badge/arch-AArch64-red.svg?style=plastic)](https://en.wikipedia.org/wiki/AArch64)

#### 介绍
1.该项目是基于C++进行编写一款智能CPU调度 <br>
2.该调度是基于schedhorizon调速器进行调整 <br>
3.该调度为ZTC内核专属调度

#### 工作条件
1.目前该调度适用于Android12-14 <br>
2.内核5.10-5.15

## 安装
1.下载后通过Magisk Manager刷入，Magisk版本不低于20.4 <br>
#### 修改启动时的默认模式
1.打开/sdcard/Android/MW_CpuSpeedController/config.txt <br>
2.可选的挡位有 super_powersave powersave balance performance fast五个挡位 <br>
super_powersave 超级省电模式 开启后将大幅度限制CPU频率 推荐待机使用 <br>
powersave 省电模式 保证基本流畅的同时尽可能降低功耗 推荐日常使用 <br>
2.重启后查看/sdcard/Android/MW_CpuSpeedController/log.txt检查CS调度是否正常自启动

### 性能模式切换
balance均衡模式，比原厂略流畅的同时略省电 推荐日常使用 <br>
performance性能模式，保证费电的同时多一点流畅度 推荐游戏使用 <br>
fast极速模式，默认开启Feas或官调
## 常见问题
Q：是否对待机功耗有负面影响？<br>
A：CS调度的实现做了不少低功耗的优化，得益于C++语言所以自身运行的功耗开销很低. <br>
Q：为什么使用了CS调度功耗还是好高？ <br>
A：SOC的AP部分功耗主要取决于计算量以及使用的频点。CS只能控制性能释放，改进频率的方式从而降低功耗，如果后台APP的计算量很大是无法得到显著的续航延长的。这一问题可以通过Scene工具箱的进程管理器来定位。 <br>
Q：是否还需要关闭系统的performance boost？ <br>
A：模块内脚本已经关闭了大部分的用户态和内核态升频，如果有非常规的升频需要用户自己关闭。
## 详细介绍 
该模块使用的调速器是schedhorizon <br>
所以在部分场景中得益于schedhorizon调速器会比Powersave调速器拥有更快的响应速度、性能稳定性或资源利用率。适当的调度策略可以确保系统在不同负载下的表现良好<br>
废话不多说进入本篇的重点 配置文档

### 自定义配置文件
### 元信息

```ini
   [meta]
   name = "CS调度配置文件V2.0"
   author = "CoolApk@MoWei"
   Enable_Feas = false
   Disable_qcom_GpuBoost = false
   Core_allocation = false
   APP_freq_uency_upgrade = false

```
| 字段名   | 数据类型 | 描述                                           |
| -------- | -------- | ---------------------------------------------- |
| name     | string   | 配置文件的名称和版本                                 |
| author   | string   | 配置文件的作者信息                             |
| Working_mode | string   | CS调度的工作调速器，目前该字段不起任何作用 |
| Enable_Feas | bool   | 开启此功能后再开启极速模式就会恢复walt调速器等数据开启Feas |
| Disable_qcom_GpuBoost | bool   | 开启后将会关闭高通的GPUBoost 防止GPUBoost乱升频 |
| Core_allocation | bool   | 核心绑定 开启后将会调整CPUset |
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
- @XShe 
- @Timeline 
- @长虹久奕
- @RalseiNEO
# 使用的开源项目
[作者:wme7 项目:INIreader](https://github.com/wme7/INIreader) <br>
[作者:MoWei 项目:负载智能升频](https://github.com/MoWei-2077/CS-load-up-conversion) <br>
感谢所有用户的测试反馈 这将推进CPU调速器(CS调度)的开发

### 该文档更新于:2024/8/30 9:35
