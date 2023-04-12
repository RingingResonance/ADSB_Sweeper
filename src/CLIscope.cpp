/**
    Copyright (c) Jarrett Cigainero 2023

    This file is part of ADS-B Sweeper.

    Picture To Vector Wave is free software: you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation,
    either version 3 of the License, or (at your option) any later version.

    Picture To Vector Wave is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with Picture To Vector Wave.
    If not, see <https://www.gnu.org/licenses/>.

**/

#include "./headers/ADSBdata.h"
#include "./headers/CLIscope.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <cmath>
#include <thread>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

bool runCLIscope = 0;

int CLIrScope(void){
    if(!runCLIscope)return 0;
    while(runCLIscope){
    std::cout << "\x1B[2J\x1B[H";
    char airplane[3001];
    char border[100];
    for (int i = 0; i < 100; i++) border[i] = '#';
    border[99] = 0;
    for (int i = 0; i < 3000; i++) airplane[i] = ' ';
    for (int i = 0; i < MaxAircraft; i++) {
      if (O_ADSB_Database[i].AircraftAsleepTimer) {
        float tempX = (O_ADSB_Database[i].CALC_Xdistance * zoomFactorX) + 50;
        float tempY = (O_ADSB_Database[i].CALC_Ydistance * -zoomFactorY) + 15;
        int adrX = tempX;
        int adrY = tempY;
        if (adrX < 0) adrX = 0;
        if (adrX > 98) adrX = 98;
        if (adrY < 0) adrY = 0;
        if (adrY > 28) adrY = 28;
        if (O_ADSB_Database[i].ADSB_HDG >= 0) {
          if (O_ADSB_Database[i].ADSB_HDG >= 337 || O_ADSB_Database[i].ADSB_HDG < 22) airplane[((adrY - 1) * 100) + adrX] = '|';
          if (O_ADSB_Database[i].ADSB_HDG >= 22 && O_ADSB_Database[i].ADSB_HDG < 67) airplane[((adrY - 1) * 100) + (adrX + 1)] = '/';
          if (O_ADSB_Database[i].ADSB_HDG >= 67 && O_ADSB_Database[i].ADSB_HDG < 112) airplane[(adrY * 100) + (adrX + 1)] = '-';
          if (O_ADSB_Database[i].ADSB_HDG >= 112 && O_ADSB_Database[i].ADSB_HDG < 157) airplane[((adrY + 1) * 100) + (adrX + 1)] = '\\';
          if (O_ADSB_Database[i].ADSB_HDG >= 157 && O_ADSB_Database[i].ADSB_HDG < 202) airplane[((adrY + 1) * 100) + adrX] = '|';
          if (O_ADSB_Database[i].ADSB_HDG >= 202 && O_ADSB_Database[i].ADSB_HDG < 247) airplane[((adrY + 1) * 100) + (adrX - 1)] = '/';
          if (O_ADSB_Database[i].ADSB_HDG >= 247 && O_ADSB_Database[i].ADSB_HDG < 292) airplane[(adrY * 100) + (adrX - 1)] = '-';
          if (O_ADSB_Database[i].ADSB_HDG >= 292 && O_ADSB_Database[i].ADSB_HDG < 337) airplane[((adrY - 1) * 100) + (adrX - 1)] = '\\';
        }
        airplane[(adrY * 100) + adrX] = DSP_ID[i];
      }
    }
    for (int y = 0; y < 30; y++) {
      airplane[(y * 100) + 98] = '#';
      airplane[(y * 100) + 99] = '\n';
    }
    airplane[(15 * 100) + 50] = '@'; //Mark Center
    airplane[3000] = 0;
    std::cout << airplane << "\n";
    std::cout << border << "\n";
    std::cout << "Aircraft Loc Averaged: " << LocAvrgCnt << " Center Longitude: " << homeLon << " Center Latitude: " << homeLat << "\n";
    ///Print aircraft list
    int ni = 0;
    for (int i = 0; i < MaxAircraft; i++) {
      if (O_ADSB_Database[i].AircraftAsleepTimer) {
        if(i>25)ni=26; else ni=i;
        std::cout << DSP_ID[ni] << ":  " <<
          O_ADSB_Database[i].ADSB_Address << "  SQK:" << O_ADSB_Database[i].ADSB_Squawk <<
          "  H:" << O_ADSB_Database[i].ADSB_HDG << "  V:" << O_ADSB_Database[i].ADSB_SPD <<
          "  X:" << O_ADSB_Database[i].CALC_Xdistance << "  Y:" << O_ADSB_Database[i].CALC_Ydistance << "\n";
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    ///Kill all other threads if we crash.
    ADSBpRun=0;
    ADSBgRun=0;
    runDACscope=0;
    std::cout << "CLI scope thread terminated. \n";
    return 0;
}
