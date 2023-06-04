#!/bin/bash
<<<<<<< HEAD
dump1090-mutability | /home/pi/XYaudio/software/adsbSweeper/adsbSweeper -D -C -R 5 -m 5
PID=$(pgrep --newest adsbSweeper)
sudo renice -n -20 -p $PID
=======
sudo cpufreq-set -u 1.2GHz -d 1.2GHz
sleep 2
dump1090-mutability | /home/pi/XYaudio/software/adsbSweeper/adsbSweeper -D -a 32.831901 -o -97.088838 -R 3 -m 3 -s 400 -B 2 &
sleep 2
PID=$(pgrep --newest adsbSweeper)
sudo renice -n -18 -p $PID
>>>>>>> 1ee447ab20aa34b8c6bd9134939402b651b93fb5
