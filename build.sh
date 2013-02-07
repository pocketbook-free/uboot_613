#!/bin/sh

set -x

export PATH=/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin:$PATH

which ccache &>/dev/null
if [ $? -eq 0 ]
then
	CPREFIX="ccache arm-fsl-linux-gnueabi-"
else
	CPREFIX="arm-fsl-linux-gnueabi-"
fi

# adds hg revision to uboot
echo -n "." > localversion-1
hg id -i >> localversion-1

if [ -n "$1" && "$1" != "exit" ]
then 
	TARGET=$1
	OUT_TARGET=${TARGET%".i"}"_cpp.c"
	make ARCH=arm CROSS_COMPILE="$CPREFIX" $1
	file_extention=$(basename "$TARGET" | sed 's/^.*\.//')
	if [ $file_extention != "i" ] ; then exit 0 ; fi
	cat $TARGET | grep -v ^# | uniq > $OUT_TARGET
	astyle $OUT_TARGET
	exit 0
fi

make ARCH=arm CROSS_COMPILE="$CPREFIX" mrproper
if [ "$1" = "exit" ] ; then exit ; fi
make ARCH=arm CROSS_COMPILE="$CPREFIX" ep8_config
make ARCH=arm CROSS_COMPILE="$CPREFIX" tags
make ARCH=arm CROSS_COMPILE="$CPREFIX" -j 2
