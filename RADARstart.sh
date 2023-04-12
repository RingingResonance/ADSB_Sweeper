#!/bin/bash
sudo cpufreq-set -u 1.2GHz -d 1.2GHz
sleep 2
dump1090-mutability | /home/pi/XYaudio/software/adsbSweeper/adsbSweeper -D -a 32.831901 -o -97.088838 -R 3 -m 3 -s 400 -B 2 &
sleep 2
PID=$(pgrep --newest adsbSweeper)
sudo renice -n -18 -p $PID
