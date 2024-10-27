propPath=$1
version=$(cat $propPath | grep "version=" | cut -d "=" -f2)
versionCode=$(cat $propPath | grep "versionCode=" | cut -d "=" -f2)

json=$(
	cat <<EOF
{
    "name": "CS调度",
    "author": "MoWei/ztc1997",
    "version": "v9.7.0",
    "versionCode": 20241021,
    "features": {
        "strict": true,
        "pedestal": true
    },
    "module": "MW_CpuSpeedController",
    "state": "/sdcard/Android/MW_CpuSpeedController/config.txt",
    "entry": "/data/powercfg.sh",
    "projectUrl": "https://github.com/MoWei-2077/CPU-speed-controller"
}
EOF
)
