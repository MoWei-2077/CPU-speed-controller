MODDIR=${0%/*}

KERNEL_VERSION=`uname -r| sed -n 's/^\([0-9]*\.[0-9]*\).*/\1/p'`

insmod $MODDIR/$KERNEL_VERSION/cpufreq_schedhorizon.ko