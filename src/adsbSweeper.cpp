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

#include "./headers/adsbSweeper.h"
#include "./headers/ADSBdata.h"
#include "./headers/CLIscope.h"
#include "./headers/DACscope.h"

#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>


C_ADSB_Database* O_ADSB_Database;

int main(int argc, char * argv[]) {
    if(GetArgs(argc, argv))return 0;
    if(!runDACscope&&!runCLIscope){
        std::cout << "No output selected. No point in running. \n";
        return 0;
    }
  std::cout << "Starting ADS-B SWEEPER. \n";
  O_ADSB_Database = new C_ADSB_Database [MaxAircraft]; //Max aircraft.
  std::thread DACscopeThread(F_DACscope);       ///SPI Digital to Analog output thread to CRT.
  std::thread CLIscopeThread(CLIrScope);        ///Command-line scope thread.
  std::thread ADSBpredThread(F_ADSBpred);       ///ADS-B interpolation thread.
  std::thread ADSBgetterThread(F_ADSBgetter);   ///ADS-B input thread.
/****************************************************/
  while (runDACscope||runCLIscope){std::this_thread::sleep_for(std::chrono::microseconds(10));}///Try to sleep for most of the time.
  DACscopeThread.join();
  CLIscopeThread.join();
  ADSBgetterThread.join();
  ADSBpredThread.join();
  return 0;
}

int GetArgs(int argc, char **argv){
  float argNumber = 0;
  for(int i=1;i<argc;i++){
    if(argv[i][0]=='-'){
        if(argv[i][1]!='i' && argv[i][1]!='h' && argv[i][1]!='T' &&
           argv[i][1]!='U' && argv[i][1]!='X' && argv[i][1]!='Y'){
            std::stringstream wasd(&argv[i][3]);
            wasd >> argNumber;
        }
        switch (argv[i][1]){
            case 'D':       ///DAC scope enable.
                runDACscope = 1;
                break;
            case 't':       ///DAC scope; use half trace length.
                fullTrace = 0;
                break;
            case 'C':       ///CLI scope enable.
                runCLIscope = 1;
                break;
            case 'I':       ///CLI scope reduced info.
                CfullScope = 0;
                break;
            case 'm':       ///Max aircraft range.
                if(argNumber>0&&argNumber<=10000)MaxRange=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Max Range: " << &argv[i][0] << ", limit 0 - 10000\n"; return 1;}
                break;
            case 'a':       ///Manual Latitude setting.
                if(argNumber>=0&&argNumber<=90){
                    homeLat=argNumber;
                    MaxAvrgCnt=-1;      ///Disable auto location.
                }
                else {std::cout << "\nArgument Value Out Of Range For Latitude: " << &argv[i][0] << ", limit 0 - 90\n"; return 1;}
                break;
            case 'o':       ///Manual Longitude setting.
                if(argNumber>=-180&&argNumber<=180){
                    homeLon=argNumber;
                    MaxAvrgCnt=-1;      ///Disable auto location.
                }
                else {std::cout << "\nArgument Value Out Of Range For Longitude: " << &argv[i][0] << ", limit -180 - 180\n"; return 1;}
                break;
            case 'v':       ///Aircraft Average Count.
                if(argNumber>=0&&argNumber<=256000)MaxAvrgCnt=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Average Count: " << &argv[i][0] << ", limit 0 - 256000\n"; return 1;}
                break;
            case 'S':       ///Aircraft Sleep Timer.
                if(argNumber>=0&&argNumber<=256000)SleepTimer=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Sleep Timer: " << &argv[i][0] << ", limit 0 - 256000\n"; return 1;}
                break;
            case 'M':       ///Maximum aircraft.
                if(argNumber>=0&&argNumber<=256000)MaxAircraft=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Max Aircraft: " << &argv[i][0] << ", limit 0 - 256000\n"; return 1;}
                break;
            case 'R':       ///Radius of Scope Trace in nm.
                if(argNumber>=0&&argNumber<=10000)distFactor=512/argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Trace Length: " << &argv[i][0] << ", limit 0 - 10000\n"; return 1;}
                break;
            case 'B':       ///Blip Scale.
                if(argNumber>=0&&argNumber<=1000)blipScale=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Blip Scale: " << &argv[i][0] << ", limit 0 - 1000\n"; return 1;}
                break;

            case 'b':       ///Blip Brightness.
                if(argNumber>=0&&argNumber<=1023)blipInten=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Blip Brightness: " << &argv[i][0] << ", limit 0 - 1023\n"; return 1;}
                break;

            case 'd':       ///Dim Trace Brightness.
                if(argNumber>=0&&argNumber<=1023)dimInten=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Trace Brightness: " << &argv[i][0] << ", limit 0 - 1023\n"; return 1;}
                break;

            case 's':       ///Scale Brightness.
                if(argNumber>=0&&argNumber<=1023)scaleInten=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Scale Brightness: " << &argv[i][0] << ", limit 0 - 1023\n"; return 1;}
                break;

            case 'c':       ///Blanking Brightness.
                if(argNumber>=0&&argNumber<=1023)blankInten=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Blanking: " << &argv[i][0] << ", limit 0 - 1023\n"; return 1;}
                break;
            case 'f':       ///SPI frequency
                if(argNumber>=0&&argNumber<=32000000)SPIfreq=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Blanking: " << &argv[i][0] << ", limit 0 - 32000000\n"; return 1;}
                break;
            case 'w':       ///SPI delay in uSec
                if(argNumber>=0&&argNumber<=999)SPIdly=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For Blanking: " << &argv[i][0] << ", limit 0 - 999\n"; return 1;}
                break;
            case 'h':       ///Help Text.
                std::cout << helpText[0]; return 1;
                break;
            case 'r':       ///CLI scope refresh in ms.
                if(argNumber>=0&&argNumber<=10000)refTime=argNumber;
                else {std::cout << "\nArgument Value Out Of Range For CLI refresh: " << &argv[i][0] << ", limit 0 - 10000\n"; return 1;}
                break;
        }
    }
  }
  return 0;
}
