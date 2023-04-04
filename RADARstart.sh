#!/bin/bash
dump1090-mutability | ./adsbSweeper -D -C -a 32.757541 -o -97.076364 -R 5 -m 5
PID=$(pgrep --newest adsbSweeper)
sudo renice -n -20 -p $PID
