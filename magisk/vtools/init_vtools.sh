BASEDIR="$(dirname $(readlink -f "$0"))"
source $BASEDIR/gen_json.sh $1
echo "$json" >/data/powercfg.json
cp -af $BASEDIR/powercfg.sh /data/powercfg.sh
chmod 755 /data/powercfg.sh
mkdir /sdcard/Android/MW_CpuSpeedController
cur_powermode="/sdcard/Android/MW_CpuSpeedController/config.txt"
if [ ! -f "$cur_powermode" ]; then
	touch "$cur_powermode"
	echo "powersave" > "$cur_powermode"
fi
