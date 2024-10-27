clear(){
    rm -rf /sdcard/Android/MW_CpuSpeedController
    sleep 0.2
    mkdir /sdcard/Android/MW_CpuSpeedController
}
enforce_install_from_magisk_app() {
echo "欢迎使用CS调度"
echo "该模块目前仅支持5.10-5.15的内核 在ZTC内核上使用此调度相交于普通内核优化要好"
echo "该项目目前基于coolapk@ztc1997的schedhorizon模块制作"
echo "感谢:
CookApk@:ztc1997
CookApk@:Nep_Timeline
CoolApk@:XShee
CoolApk@:RalseiNEO
QQ@长虹久奕.
提供的技术支持"

cat <<EOF > "/sdcard/Android/MW_CpuSpeedController/config.ini"
[meta]
name = "CS调度配置文件V4.0"
author = "CoolApk@MoWei"
Enable_Feas = false
Disable_qcom_GpuBoost = false
Core_allocation = false
Load_balancing = false
Disable_UFS_clock_gate = false
TouchBoost = false
CFS_Scheduler = false
EOF
}
fullApkPath=$(ls "$MODPATH"/CS-Controller_v*.apk)
# shellcheck disable=SC2125
apkPath=$TMPDIR/CS-Controller_v*.apk
mv -f "$fullApkPath" "$apkPath"
chmod 666 "$apkPath" 

echo "- CSController 正在安装..."
output=$(pm install -r -f "$apkPath" 2>&1)
if [ "$output" == "Success" ]; then
    echo "- CSController 安装成功"
    rm -rf "$apkPath"
else
    echo "- CSController 安装失败, 原因: [$output] 尝试卸载再安装..."
    pm uninstall io.github.xsheeee.cs_controller
    sleep 1
    output=$(pm install -r -f "$apkPath" 2>&1)
    if [ "$output" == "Success" ]; then
        echo "- CSController 安装成功"
        rm -rf "$apkPath"
    else
        apkPathSdcard=/sdcard/CSController_${module_version}.apk
        cp -f "$apkPath" "$apkPathSdcard"
        echo "!!! *********************** !!!"
        echo "  CSController 依旧安装失败, 原因: [$output]"
        echo "  请手动安装 [ $apkPathSdcard ]"
        echo "  如果是降级安装, 请手动卸载CSController, 然后再次安装。"
        echo "!!! *********************** !!!"
    fi
fi

Init(){
sh $MODPATH/vtools/init_vtools.sh $(realpath $MODPATH/module.prop)
}
clear
enforce_install_from_magisk_app
Init