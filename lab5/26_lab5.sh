
LD_LIBRARY_PATH=./lib ./madplay-lib/madplay ./music.mp3 -o wav:- | LD_LIBRARY_PATH=./lib  ./alsa-utils/bin/aplay

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
./madplay-lib/madplay ./music.mp3 -o wav:- | ./alsa-utils/bin/aplay

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/run/media/mmcblk1p1/lab5/lib
./madplay-lib/madplay ./music.mp3 -o wav:- | ./alsa-utils/bin/aplay

LD_LIBRARY_PATH=./lib ./madplay-lib/madplay ./music.mp3 -o wav:- > hahaha.wav
LD_LIBRARY_PATH=./lib  ./alsa-utils/bin/aplay hahaha.wav
