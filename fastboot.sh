#!/bin/sh
#fastboot.sh


sudo dd if=u-boot.bin of=u-boot-new.bin bs=1K skip=1
sudo chmod 777 u-boot-new.bin
sudo fastboot flash uboot u-boot-new.bin
sudo fastboot reboot

echo ok
sudo fastboot reboot
