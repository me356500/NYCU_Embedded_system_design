#!/bin/sh
### BEGIN INIT INFO
# Provides:          xlpenix
# Required-Start:    $local_fs $network $named $time $syslog
# Required-Stop:     $local_fs $network $named $time $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Description:       lab4_bonus
### END INIT INFO

LD_LIBRARY_PATH=./run/media/mmcblk1p1/wallpaper/ ./run/media/mmcblk1p1/wallpaper/demo ./run/media/mmcblk1p1/wallpaper/pepe.png 