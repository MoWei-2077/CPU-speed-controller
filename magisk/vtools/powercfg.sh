#!/system/bin/sh



switch_mode() {
    echo "$1" > /sdcard/Android/MW_CpuSpeedController/config.txt
}

case $1 in
    "powersave" | "balance" | "performance" | "fast")
        switch_mode $1
        ;;
    "init")
        /data/powercfg.sh $(cat /sdcard/Android/MW_CpuSpeedController/config.txt)
        ;;
    *)
        echo "Failed to apply unknown action '$1'."
        ;;
esac