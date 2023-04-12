#!/bin/bash
dump1090-mutability | /home/pi/XYaudio/software/adsbSweeper/adsbSweeper -D -a 32.831901 -o -97.088838 -R 3 -m 3 -s 300 &
sleep 2
sudo cpufreq-set -u 1.2GHz -d 1.2GHz
PID=$(pgrep --newest adsbSweeper)
sudo renice -n -18 -p $PID

sleep 1
PID=$(pgrep --newest vlc)
sudo renice -n 7 -p $PID
