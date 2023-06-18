#!/bin/bash
dump1090-mutability --max-range 4 | sudo cset shield --exec /home/pi/XYaudio/software/adsbSweeper/adsbSweeper -- -w 10 -f 8000000 -D -d 530 -C -I -r 5000 -R 5 -m 5
PID=$(pgrep --newest adsbSweeper)
sudo renice -n -20 -p $PID
