#!/bin/bash
PIDadsb=$(pgrep --newest adsbSweeper)
PIDd1090=$(pgrep --newest dump1090-mutabi)
sudo kill -9 $PIDadsb $PIDd1090
