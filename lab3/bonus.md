## init.d (before login)

### steps

1. ```cp record.sh /etc/init.d/record.sh```
2. ```sudo chmod 777 record.sh```
3. ```sudo update-rc.d record.sh defaults```

### remove

```sudo update-rc.d -f record.sh remove```

### runlevel

0   : Halt.
1   : Single user mode, no internet, no root.
2   : Multi user mode, no internet.
3   : Multi user mode, default startup.
4   : User customize.
5   : Multi user mode with GUI.
6   : Reboot.

## bashrc (after login)

### steps

1. ```vim ~/.bashrc```
2. ```killall record```
3. ```LD_LIBRARY_PATH=/run/media/mmcblk1p1/lab3/ /run/media/mmcblk1p1/lab3/record after.avi```