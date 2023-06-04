#!/bin/bash
dump1090-mutability | /home/pi/XYaudio/software/adsbSweeper/adsbSweeper -D -C -R 5 -m 5
PID=$(pgrep --newest adsbSweeper)
sudo renice -n -20 -p $PID
