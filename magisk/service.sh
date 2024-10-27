#!/system/bin/sh
# 请不要硬编码/magisk/modname/...;相反，请使用$MODDIR/...
# 这将使您的脚本兼容，即使Magisk以后改变挂载点
MODDIR=${0%/*}
# 该脚本将在设备开机后作为延迟服务启动
wait_until_login() {

    # in case of /data encryption is disabled
    while [ "$(getprop sys.boot_completed)" != "1" ]; do
        sleep 0.1
    done

    # we doesn't have the permission to rw "/sdcard" before the user unlocks the screen
    # shellcheck disable=SC2039
    local test_file="/sdcard/Android/.PERMISSION_TEST_FREEZEIT"
    true >"$test_file"
    while [ ! -f "$test_file" ]; do
        sleep 2
        true >"$test_file"
    done
    rm "$test_file"
}
lock_val() {
    chmod 0666 "$2"
    echo "$1" >"$2"
    chmod 0444 "$2"
}
init_node_mtk() {
    lock_val "0" /sys/kernel/ged/hal/fastdvfs_mode
    lock_val "0" /sys/kernel/ged/hal/custom_upbound_gpu_freq
    lock_val "0" /sys/module/mtk_core_ctl/policy_enable
    lock_val "0" /sys/kernel/fpsgo/fbt/switch_idleprefer
    lock_val "0" /sys/module/mtk_fpsgo/parameters/boost_affinity

    lock_val "0" /sys/devices/platform/utos/tzdriver_current_mode
    lock_val "0" /sys/class/thermal/thermal_message/tzdriver_current_mode
    
    stop power-hal-1-0 && start power-hal-1-0
    
	pm uninstall --user 0 com.mediatek.duraspeed
    
	killall -9 com.mediatek.duraspeed

	stop aee.log-1-1
	stop vendor_tcpdump
	stop emdlogger
	stop consyslogger
	stop mobile_log_d
	stop wifi_dump
	stop bt_dump
    echo "true" > /data/adb/modules/MW_CpuSpeedController/MTKPhone
}

init_platform_config() {
	if [ "$(getprop ro.hardware)" != "qcom" ]; then
		init_node_mtk
	fi
}
Init_schedhorizon(){
    sh $MODDIR/vtools/init_vtools.sh $(realpath $MODDIR/module.prop)
    /data/powercfg.sh $(cat /data/cur_powermode.txt)
}
wait_until_login
init_platform_config
Init_schedhorizon
stop vendor.oplus.ormsHalService-aidl-default
chmod 777 $MODDIR/*
/data/powercfg.sh $(cat "/sdcard/Android/MW_CpuSpeedController/config.txt")
chmod 777 /data/adb/modules/MW_CpuSpeedController/system/bin/*
su -c /data/adb/modules/MW_CpuSpeedController/system/bin/MW_CpuSpeedController