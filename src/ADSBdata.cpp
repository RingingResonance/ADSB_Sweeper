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
bool    LocAvrgGet = 0;
int  sweeptimer = 0;
bool addressFound = NotFound;
bool ADSBgRun = 1;
bool ADSBpRun = 1;
bool latFound = NotFound;
bool lonFound = NotFound;
float MaxRange = 4;
int MaxAvrgCnt = 10;
int SleepTimer = 10;
int MaxAircraft = 26;

/** ADS-B loop **/
/// Searches through stdin line-by-line as the data comes in.
void F_ADSBgetter() {
  std::cout << "Starting ADS-B Receiver Thread. \n";
  char ADSB_MESSAGE[1000];
  char ICAOadr[20];
  int ICAOsquak;
  float ICAOheading;
  float ICAOspeed;
  float ICAOlat = 0;
  float ICAOlon = 0;
  while (ADSBgRun&&(runDACscope||runCLIscope)) {
    std::cin.clear();               ///Clear buffer.
    std::cin >> &ADSB_MESSAGE[0];   ///Get new line of text to process.
    ///Look for start of message.
    if (ADSB_MESSAGE[0] == '*') {
      addressFound = NotFound;
      latFound = NotFound;
      lonFound = NotFound;
    }
    ///Look for ICAO address.
    if (addressFound == NotFound) {
      for (int i = 0; i < 50 && addressFound != Found; i++) {
        if (ADSB_MESSAGE[i] == ICAO_ADR[i]) {
          if (ADSB_MESSAGE[i] == ':') {
            std::cin >> & ICAOadr[0];
            addressFound = Found;
            break;
          }
        }
      }
    }
    ///If further into the message and we have already found the address...
    else if (addressFound == Found) {
        ///Look for info.
      ICAOsquak = -1; ICAOheading = -1; ICAOspeed = -1;
      int typeSelector = 0xFF; int ICAO_index = 0; bool infoFound = NotFound;
      for (int i = 0; i < 50; i++) {
        char CC = ADSB_MESSAGE[i];
        if(CC!=' ' || infoFound){
            if ((typeSelector&typeLat) && CC==ICAO_LAT[ICAO_index] && !latFound){
                infoFound = Found;
                if (CC == ':') {
                    std::cin >> ICAOlat;
                    if (ICAOlat != 0 && ICAOlat < 91 && ICAOlat > -91) {
                        latFound = Found;
                        break;
                    }
                }
            }
            else if ((typeSelector&typeLon) && CC==ICAO_LON[ICAO_index] && !lonFound){
                infoFound = Found;
                typeSelector&=(typeLat^0xFF);
                if (CC == ':') {
                    std::cin >> ICAOlon;
                    if (ICAOlon != 0 && ICAOlon < 181 && ICAOlon > -181) {
                        lonFound = Found;
                        break;
                    }
                }
            }
            else if ((typeSelector&typeSqk) && CC==ICAO_SQK[ICAO_index]){
                infoFound = Found;
                typeSelector&=(typeLon^0xFF);
                if (CC == ':') {
                    std::cin >> ICAOsquak;
                    break;
                }
            }
            else if ((typeSelector&typeHdg) && CC==ICAO_HDG[ICAO_index]){
                infoFound = Found;
                typeSelector&=(typeSqk^0xFF);
                if (CC == ':') {
                    std::cin >> ICAOheading;
                    break;
                }
            }
            else if ((typeSelector&typeSpd) && CC==ICAO_SPD[ICAO_index]){
                infoFound = Found;
                typeSelector&=(typeHdg^0xFF);
                if (CC == ':') {
                    std::cin >> ICAOspeed;
                    break;
                }
            }
            else {
                typeSelector==0;
                break;
            }
            ICAO_index++;
        }
      }
      /** Now put the found information into the database somewhere. **/
      bool nameFound = 0;
      int AircraftNum = -1;
      /** Search for same airplane or for a sleeping airplane. **/
      for (int i = 0; i<MaxAircraft && AircraftNum<0; i++) {
        /** Search name **/
        for (int n = 0; n < 20; n++) {
            if (O_ADSB_Database[i].ADSB_Address[n] != ICAOadr[n]) {
                nameFound = NotFound;
                break;
            }
            else if (n==5){
                nameFound = Found;
                AircraftNum = i;
                break;
            }
        }
      }
      ///If name not in database then find a sleeping plane to replace.
      for (int i = 0; i<MaxAircraft&&nameFound==NotFound; i++) {
          if (O_ADSB_Database[i].AircraftAsleepTimer == Sleeping) {
                AircraftNum = i;
                break;
          }
      }
        /** If name is the same then we update it's information. **/
        if (AircraftNum != -1) {
          ///Update address if name wasn't found.
          for (int n = 0; n<20 && !nameFound; n++) {
              ///std::this_thread::sleep_for(std::chrono::microseconds(2));///Wait a little. We don't need to run *that* fast.
              O_ADSB_Database[AircraftNum].ADSB_Address[n] = ICAOadr[n]; //Update the address/name.
          }
          ///Check if we got lat/lon data.
          if (lonFound && latFound) {
            ///Get average location of X number of new aircraft.
            if(LocAvrgCnt>-1 && LocAvrgCnt<MaxAvrgCnt && O_ADSB_Database[AircraftNum].ADSB_preXY == 0){
                O_ADSB_Database[AircraftNum].ADSB_preXY = 1;
                avrgLon += ICAOlon;
                avrgLat += ICAOlat;
                LocAvrgCnt++;
            }
            else if(LocAvrgCnt==MaxAvrgCnt && !LocAvrgGet){
                homeLon = avrgLon/MaxAvrgCnt;
                homeLat = avrgLat/MaxAvrgCnt;
                LocAvrgGet = 1;
            }
            ///Update Lat&Long
            O_ADSB_Database[AircraftNum].CALC_Ydistance = O_ADSB_Database[AircraftNum].ADSB_Ydistance = (ICAOlat - homeLat) * 59.71922; //convert to nautical
            float ABShomeLat = homeLat;
            if (ABShomeLat < 0) ABShomeLat *= -1; //get absolute value of homeLat
            O_ADSB_Database[AircraftNum].CALC_Xdistance = O_ADSB_Database[AircraftNum].ADSB_Xdistance = (cos(ABShomeLat) * 60.09719) * (ICAOlon - homeLon);
            O_ADSB_Database[AircraftNum].ADSB_preXY = 1;
            O_ADSB_Database[AircraftNum].CALC_timer = interpCycles;
          }
          else if(!nameFound)O_ADSB_Database[AircraftNum].ADSB_preXY = 0; ///If new aircraft and haven't gotten lat and lon data, then reset pre_XY
          ///Update or Reset Squawk, Heading, or Speed data
            if (ICAOsquak > -1) O_ADSB_Database[AircraftNum].ADSB_Squawk = ICAOsquak;
            else if (!nameFound) O_ADSB_Database[AircraftNum].ADSB_Squawk = -1; ///If it's a new name and not getting a squawk code then reset it to 0
            ///Update Heading
            if (ICAOheading > -1) O_ADSB_Database[AircraftNum].ADSB_HDG = ICAOheading;
            else if (!nameFound) O_ADSB_Database[AircraftNum].ADSB_HDG = -1;
            ///Update Speed
            if (ICAOspeed > -1) O_ADSB_Database[AircraftNum].ADSB_SPD = ICAOspeed;
            else if (!nameFound) O_ADSB_Database[AircraftNum].ADSB_SPD = -1;
          /// If we get an update then reset SleepTimer, but only if we have already gotten at least one geo-cord.
          if (O_ADSB_Database[AircraftNum].ADSB_preXY) O_ADSB_Database[AircraftNum].AircraftAsleepTimer = SleepTimer;
          ///If out of range then remove from list via setting SleepTimer to 'Sleeping' unless we are still gathering averages to get our working location.
          if ((ABSfloat(O_ADSB_Database[AircraftNum].CALC_Ydistance) > MaxRange ||
               ABSfloat(O_ADSB_Database[AircraftNum].CALC_Xdistance) > MaxRange) &&
               LocAvrgCnt>=MaxAvrgCnt) O_ADSB_Database[AircraftNum].AircraftAsleepTimer = Sleeping;
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
          ///If out of range then remove from list via setting SleepTimer to Asleep, unless we are still gathering averages.
          if ((ABSfloat(O_ADSB_Database[i].CALC_Ydistance) > MaxRange ||
               ABSfloat(O_ADSB_Database[i].CALC_Xdistance) > MaxRange) &&
               LocAvrgCnt>=MaxAvrgCnt) O_ADSB_Database[i].AircraftAsleepTimer = Sleeping;

        }
        else O_ADSB_Database[i].CALC_timer--;
        ///Decrease sleep timer once per second.
        if(O_ADSB_Database[i].sleepMult>interpCyclesPerSec){
            O_ADSB_Database[i].AircraftAsleepTimer--;
            O_ADSB_Database[i].sleepMult=0;
        }
        else O_ADSB_Database[i].sleepMult++;
      }
      ///std::this_thread::sleep_for(std::chrono::microseconds(2));///Wait a little. We don't need to run *that* fast.
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
