# init.d

## steps

1. ```cp wallpaper.sh /etc/init.d/wallpaper.sh```
2. ```sudo chmod 777 wallpaper.sh```
3. ```sudo update-rc.d wallpaper.sh defaults```

## remove

```sudo update-rc.d -f wallpaper.sh remove```

## runlevel

0   : Halt.
1   : Single user mode, no internet, no root.
2   : Multi user mode, no internet.
3   : Multi user mode, default startup.
4   : User customize.
5   : Multi user mode with GUI.
6   : Reboot.