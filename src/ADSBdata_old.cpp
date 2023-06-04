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
#include "./headers/DACscope.h"
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

struct timespec slptm;

double  homeLon = 0;
double  homeLat = 0;
double  avrgLon = 0;
double  avrgLat = 0;
int     LocAvrgCnt = 0;
int  sweeptimer = 0;
bool addressFound = NotFound;
bool ADSBgRun = 1;
bool ADSBpRun = 1;
bool latFound = NotFound;
bool lonFound = NotFound;
float MaxRange = 4;
int MaxAvrgCnt = 100;
int SleepTimer = 10;
int MaxAircraft = 26;

/** Please notice the delays implemented in various portions of this program. They are for slowing down the program so
    as to not use all resources at once in order to keep from disturbing DACscope's threads. **/
/** ADS-B loop **/
void F_ADSBgetter() {
  std::cout << "Starting ADS-B Receiver Thread. \n";
  char ADSB_MESSAGE[1000];
  char ICAOadr[20];
  int ICAOsquak;
  float ICAOheading;
  float ICAOspeed;
  float ICAOlat;
  float ICAOlon;
  while (ADSBgRun&&(runDACscope||runCLIscope)) {
    for (int i = 0; i < 1000; i++) {
        ADSB_MESSAGE[i] = 0;
        std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
    }
    std::cin.clear();
    std::cin >> & ADSB_MESSAGE[0];
    //Look for start of message.
    if (ADSB_MESSAGE[0] == '*') {
      addressFound = NotFound;
      latFound = NotFound;
      lonFound = NotFound;
    }
    //Look for ICAO address or latitude/longitude
    if (addressFound == NotFound) {
      //cout << "Looking for address. \n";
      for (int i = 0; i < 50 && addressFound != Found; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
        if (ADSB_MESSAGE[i] == ICAO_ADR[i]) {
          if (ADSB_MESSAGE[i] == ':') {
            std::cin >> & ICAOadr[0];
            addressFound = Found;
            break;
          }
        }
      }
    } else if (addressFound == Found) {
      for (int i = 0; i < 50 && !latFound; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
        if (ADSB_MESSAGE[i] == ICAO_LAT[i]) {
          if (ADSB_MESSAGE[i] == ':') {
            std::cin >> ICAOlat;
            if (ICAOlat != 0) {
              latFound = Found;
            }
            break;
          }
        } else break;
      }
      for (int i = 0; i < 50 && !lonFound; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
        if (ADSB_MESSAGE[i] == ICAO_LON[i]) {
          if (ADSB_MESSAGE[i] == ':') {
            std::cin >> ICAOlon;
            if (ICAOlon != 0) {
              lonFound = Found;
            }
            break;
          }
        } else break;
      }
      ICAOsquak = -1;
      for (int i = 0; i < 50; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
        if (ADSB_MESSAGE[i] == ICAO_SQK[i]) {
          if (ADSB_MESSAGE[i] == ':') {
            std::cin >> ICAOsquak;
            break;
          }
        } else break;
      }
      ICAOheading = -1;
      for (int i = 0; i < 50; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
        if (ADSB_MESSAGE[i] == ICAO_HDG[i]) {
          if (ADSB_MESSAGE[i] == ':') {
            std::cin >> ICAOheading;
            break;
          }
        } else break;
      }
      ICAOspeed = -1;
      for (int i = 0; i < 50; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
        if (ADSB_MESSAGE[i] == ICAO_SPD[i]) {
          if (ADSB_MESSAGE[i] == ':') {
            std::cin >> ICAOspeed;
            break;
          }
        } else break;
      }
      /** Now put the found information into the database. **/
      bool nameFound = NotFound;
      /** Search for same airplane or for a sleeping airplane. **/
      for (int i = 0; i < MaxAircraft; i++) {
        /** Search name **/
        nameFound = Found;
        for (int n = 0; n < 20; n++) {
          if (O_ADSB_Database[i].ADSB_Address[n] != ICAOadr[n]) {
            nameFound = NotFound;
            break;
          }
          std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
        }
        /** If name is the same then we update it's location **/
        if (nameFound == Found) {
          //Update Lat&Long
          if (lonFound && latFound) {
                ///Get average location of 50 new aircraft.
              if(LocAvrgCnt>=0 && LocAvrgCnt<MaxAvrgCnt && O_ADSB_Database[i].ADSB_preXY == 0){
                O_ADSB_Database[i].ADSB_preXY = 1;
                avrgLon += ICAOlon;
                avrgLat += ICAOlat;
                LocAvrgCnt++;
              }
              else if(LocAvrgCnt==MaxAvrgCnt){
                homeLon = avrgLon/MaxAvrgCnt;
                homeLat = avrgLat/MaxAvrgCnt;
                LocAvrgCnt=MaxAvrgCnt+1;
              }
            O_ADSB_Database[i].CALC_Ydistance = O_ADSB_Database[i].ADSB_Ydistance = (ICAOlat - homeLat) * 59.71922; //convert to nautical
            float ABShomeLat = homeLat;
            if (ABShomeLat < 0) ABShomeLat *= -1; //get absolute value of homeLat
            O_ADSB_Database[i].CALC_Xdistance = O_ADSB_Database[i].ADSB_Xdistance = (cos(ABShomeLat) * 60.09719) * (ICAOlon - homeLon);
            O_ADSB_Database[i].ADSB_preXY = 1;
            O_ADSB_Database[i].CALC_timer = interpCycles;
            O_ADSB_Database[i].AircraftAsleepTimer = SleepTimer; //Reset it's sleep timer.
          }
          //Update Squawk
          if (ICAOsquak > 0) O_ADSB_Database[i].ADSB_Squawk = ICAOsquak;
          //Update Heading
          if (ICAOheading > -1) O_ADSB_Database[i].ADSB_HDG = ICAOheading;
          //Update peed
          if (ICAOspeed > -1) O_ADSB_Database[i].ADSB_SPD = ICAOspeed;
          //If we get an update then reset the timer.
          //But only if we have already gotten at least one geo-cord + heading + speed data.
          if (O_ADSB_Database[i].ADSB_preXY && O_ADSB_Database[i].ADSB_SPD > -1 && O_ADSB_Database[i].ADSB_HDG > -1)
            O_ADSB_Database[i].AircraftAsleepTimer = SleepTimer;
          //If out of range then remove from list via setting SleepTimer to Asleep;
          if (ABSfloat(O_ADSB_Database[i].CALC_Ydistance) > MaxRange ||
            ABSfloat(O_ADSB_Database[i].CALC_Xdistance) > MaxRange) O_ADSB_Database[i].AircraftAsleepTimer = Sleeping;
          break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
      }
      /** If name not found, search for a sleeping plane to replace or update **/
      if (nameFound == NotFound) {
        for (int i = 0; i < MaxAircraft; i++) {
          if (O_ADSB_Database[i].AircraftAsleepTimer == Sleeping) {
            for (int n = 0; n < 20; n++) {
              O_ADSB_Database[i].ADSB_Address[n] = ICAOadr[n]; //Update the address/name.
              std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
            }
            if (lonFound && latFound) {
              O_ADSB_Database[i].CALC_Ydistance = O_ADSB_Database[i].ADSB_Ydistance = (ICAOlat - homeLat) * 59.71922; //convert to nautical
              if (ABSfloat(O_ADSB_Database[i].ADSB_Ydistance) > MaxRange) break;
              float ABShomeLat = homeLat;
              if (ABShomeLat < 0) ABShomeLat *= -1; //get absolute value of homeLat
              O_ADSB_Database[i].CALC_Xdistance = O_ADSB_Database[i].ADSB_Xdistance = (cos(ABShomeLat) * 60.09719) * (ICAOlon - homeLon);
              if (ABSfloat(O_ADSB_Database[i].ADSB_Xdistance) > MaxRange) break;
              O_ADSB_Database[i].ADSB_preXY = 1;
              O_ADSB_Database[i].CALC_timer = interpCycles;
              O_ADSB_Database[i].AircraftAsleepTimer = SleepTimer; //Reset it's sleep timer.
            }
            /** If it's a new aircraft and we aren't getting geo-data then Reset Previous XY get. **/
            else O_ADSB_Database[i].ADSB_preXY = 0;
            /** Update Squawk Code **/
            if (ICAOsquak != 0) O_ADSB_Database[i].ADSB_Squawk = ICAOsquak;
            /** If it's a new name and not getting a squawk code then reset it to 0 **/
            else O_ADSB_Database[i].ADSB_Squawk = -1;
            //Update Heading
            if (ICAOheading != 0) O_ADSB_Database[i].ADSB_HDG = ICAOheading;
            else O_ADSB_Database[i].ADSB_HDG = -1;
            //Update Speed
            if (ICAOspeed != 0) O_ADSB_Database[i].ADSB_SPD = ICAOspeed;
            else O_ADSB_Database[i].ADSB_SPD = -1;
            break;
          }
          std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
        }
      }
    }
  }
  ADSBpRun=0;
  runCLIscope=0;
  runDACscope=0;
  std::cout << "ADS-B Receiver thread terminated. \n";
}

void F_ADSBpred() {
  std::cout << "Starting ADS-B Interp Thread. \n";
  while (ADSBpRun&&(runDACscope||runCLIscope)) {
    for (int i = 0; i < MaxAircraft; i++) {
      if (O_ADSB_Database[i].AircraftAsleepTimer != Sleeping &&
        O_ADSB_Database[i].ADSB_preXY &&
        O_ADSB_Database[i].ADSB_SPD > -1 &&
        O_ADSB_Database[i].ADSB_HDG > -1) {
        if (O_ADSB_Database[i].CALC_timer <= 0) {
          O_ADSB_Database[i].CALC_timer = interpCycles;
          O_ADSB_Database[i].CALC_Xdistance += std::sin(DegToRad(O_ADSB_Database[i].ADSB_HDG)) * (O_ADSB_Database[i].ADSB_SPD / interpFactor);
          O_ADSB_Database[i].CALC_Ydistance += std::cos(DegToRad(O_ADSB_Database[i].ADSB_HDG)) * (O_ADSB_Database[i].ADSB_SPD / interpFactor);
          //If out of range then remove from list via setting SleepTimer to Asleep;
          if (ABSfloat(O_ADSB_Database[i].CALC_Ydistance) > MaxRange ||
            ABSfloat(O_ADSB_Database[i].CALC_Xdistance) > MaxRange) O_ADSB_Database[i].AircraftAsleepTimer = Sleeping;

        }
        else O_ADSB_Database[i].CALC_timer--;
        ///Decrease sleep timer once per second.
        if(O_ADSB_Database[i].sleepMult>interpCyclesPerSec){
            O_ADSB_Database[i].AircraftAsleepTimer--;
            O_ADSB_Database[i].sleepMult=0;
        }
        else O_ADSB_Database[i].sleepMult++;
      }
      std::this_thread::sleep_for(std::chrono::microseconds(1));///Wait a little. We don't need to run *that* fast.
    }
    std::this_thread::sleep_for(std::chrono::microseconds(interpTime));
  }
  ADSBgRun=0;
  runCLIscope=0;
  runDACscope=0;
  std::cout << "ADS-B Interpolation thread terminated. \n";
}

float ABSfloat(float in ) {
  float i = 0;
  if (in < 0) i = in * -1;
  else i = in;
  return i;
}

float DegToRad(float in ) {
  return (in * PI) / 180;
}
