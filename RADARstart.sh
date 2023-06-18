#!/bin/bash
dump1090-mutability --max-range 5 | sudo cset shield --exec /home/pi/XYaudio/software/adsbSweeper/adsbSweeper -- -w 5 -f 16000000 -D -t -d 600 -C -r 8000 -R 5 -m 5
PID1=$(pgrep --newest adsbSweeper)
sudo renice -n -20 -p $PID
#taskset -c -p -a 1-3 $PID
PID=$(pgrep --newest dump1090-mutabi)
#taskset -c -p -a 0 $PID
