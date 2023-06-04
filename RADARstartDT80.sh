#!/bin/bash
dump1090-mutability | /home/pi/XYaudio/software/adsbSweeper/adsbSweeper -D -C -I -a 32.831937 -o -97.088730 -R 5 -m 5
PID=$(pgrep --newest adsbSweeper)
sudo renice -n -20 -p $PID
