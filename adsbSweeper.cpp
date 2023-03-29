/**
    Copyright (c) Jarrett Cigainero 2022

    This file is part of Picture To Vector Wave.

    Picture To Vector Wave is free software: you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation,
    either version 3 of the License, or (at your option) any later version.

    Picture To Vector Wave is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with Picture To Vector Wave.
    If not, see <https://www.gnu.org/licenses/>.

**/

#include "adsbSweeper.h"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <gpiod.h>


struct timespec slptm;

int main(int argc, char *argv[]) {
    const char *chipname = "gpiochip0";
    struct gpiod_chip *chip;
    struct gpiod_line_bulk *GPIOout;
    std::cout << "Starting. \n";
    // Open GPIO chip
    //chip = gpiod_chip_open_by_name(chipname);
    std::cout << "Got GPIO chip. \n";
    // Open GPIO lines
    //gpiod_chip_get_all_lines(chip, GPIOout);
    std::cout << "Got GPIO lines. \n";
    // Open lines for output
    //gpiod_line_request_bulk_output(GPIOout,"example1", 0);
    std::cout << "Set GPIO lines to outputs. \n";

    /** Create ADS-B database **/
    O_ADSB_Database = new C_ADSB_Database [MaxAircraft]; //Max aircraft.
    /** Create loop for display. **/
    std::thread DisplayThread( F_Display );
    /** Create loop for information predictor. **/
    std::thread ADSBpredThread( F_ADSBpred );
    /** Create loop for ADS-B information getter. **/
    std::thread ADSBgetterThread( F_ADSBgetter );
    /***********************************************/
    /** Calculate delay used for the sweep timer **/
    slptm.tv_sec = 0;
    float sweepTemp = 100000000*(1/(373760/sweepTime));
    sweeptimer = sweepTemp;
    slptm.tv_nsec = 1;
    ADSBgetterThread.join();
    ADSBpredThread.join();
    DisplayThread.join();
    //pthread_join(O_Display, NULL);
    return 0;
}
/** Display Loop **/
void F_Display(){
    while(displayRun){
        for(int radarSweep=0;radarSweep<365;radarSweep++){
            for(int radarTrace=-512;radarTrace<512;radarTrace++){
                int Xout = 512+(std::cos(radarSweep)*radarTrace);
                int Yout = 512+(std::sin(radarSweep)*radarTrace);
                //gpiod_line_set_value_bulk(GPIOout,0);
            }
        }
        std::cout << "\x1B[2J\x1B[H";
        char airplane[3001];
        char border[100];
        for(int i=0;i<100;i++)border[i]='#';border[99]=0;
        for(int i=0;i<3000;i++)airplane[i]=' ';
        for(int i=0;i<MaxAircraft;i++){
            if(O_ADSB_Database[i].AircraftAsleepTimer){
                    float tempX=(O_ADSB_Database[i].CALC_Xdistance*zoomFactorX)+50;
                    float tempY=(O_ADSB_Database[i].CALC_Ydistance*-zoomFactorY)+15;
                    int adrX=tempX;
                    int adrY=tempY;
                    if(adrX<0)adrX=0;if(adrX>98)adrX=98;
                    if(adrY<0)adrY=0;if(adrY>28)adrY=28;
                    if(O_ADSB_Database[i].ADSB_HDG>=0){
                        if(O_ADSB_Database[i].ADSB_HDG>=337||O_ADSB_Database[i].ADSB_HDG<22)airplane[((adrY-1)*100)+adrX]='|';
                        if(O_ADSB_Database[i].ADSB_HDG>=22&&O_ADSB_Database[i].ADSB_HDG<67)airplane[((adrY-1)*100)+(adrX+1)]='/';
                        if(O_ADSB_Database[i].ADSB_HDG>=67&&O_ADSB_Database[i].ADSB_HDG<112)airplane[(adrY*100)+(adrX+1)]='-';
                        if(O_ADSB_Database[i].ADSB_HDG>=112&&O_ADSB_Database[i].ADSB_HDG<157)airplane[((adrY+1)*100)+(adrX+1)]='\\';
                        if(O_ADSB_Database[i].ADSB_HDG>=157&&O_ADSB_Database[i].ADSB_HDG<202)airplane[((adrY+1)*100)+adrX]='|';
                        if(O_ADSB_Database[i].ADSB_HDG>=202&&O_ADSB_Database[i].ADSB_HDG<247)airplane[((adrY+1)*100)+(adrX-1)]='/';
                        if(O_ADSB_Database[i].ADSB_HDG>=247&&O_ADSB_Database[i].ADSB_HDG<292)airplane[(adrY*100)+(adrX-1)]='-';
                        if(O_ADSB_Database[i].ADSB_HDG>=292&&O_ADSB_Database[i].ADSB_HDG<337)airplane[((adrY-1)*100)+(adrX-1)]='\\';
                    }
                    airplane[(adrY*100)+adrX]=DSP_ID[i];
            }
        }
        for(int y=0;y<30;y++){
            airplane[(y*100)+98]='#';
            airplane[(y*100)+99]='\n';
        }
        airplane[(15*100)+50]='@';  //Mark Center
        airplane[3000]=0;
        std::cout << airplane << "\n";
        std::cout << border << "\n";
        for(int i=0;i<MaxAircraft;i++){
            if(O_ADSB_Database[i].AircraftAsleepTimer){
                std::cout << DSP_ID[i] << ":  "
                << O_ADSB_Database[i].ADSB_Address << "  SQK:" << O_ADSB_Database[i].ADSB_Squawk
                << "  H:" << O_ADSB_Database[i].ADSB_HDG << "  V:" << O_ADSB_Database[i].ADSB_SPD
                << "  X:" << O_ADSB_Database[i].CALC_Xdistance << "  Y:" << O_ADSB_Database[i].CALC_Ydistance << "\n";
                O_ADSB_Database[i].AircraftAsleepTimer--;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "Display thread terminated. \n";
}


/** ADS-B loop **/
void F_ADSBgetter(){
    char ADSB_MESSAGE[1000];
    char ICAOadr[20];
    int  ICAOsquak;
    float ICAOheading;
    float ICAOspeed;
    float ICAOlat;
    float ICAOlon;
    while(ADSBgRun){
           for (int i=0;i<1000;i++)ADSB_MESSAGE[i]=0;
           std::cin.clear();
           std::cin >> &ADSB_MESSAGE[0];
           //Look for start of message.
           if(ADSB_MESSAGE[0] == '*'){
                addressFound = NotFound;
                latFound = NotFound;
                lonFound = NotFound;
           }
           //Look for ICAO address or latitude/longitude
           if (addressFound == NotFound){
                //cout << "Looking for address. \n";
                for(int i=0;i<50&&addressFound!=Found;i++){
                    if (ADSB_MESSAGE[i] == ICAO_ADR[i]){
                        if (ADSB_MESSAGE[i] == ':'){
                            std::cin >> &ICAOadr[0];
                            addressFound = Found;
                            break;
                        }
                    }
                }
           }
           else if (addressFound == Found){
                for(int i=0;i<50&&!latFound;i++){
                    if (ADSB_MESSAGE[i] == ICAO_LAT[i]){
                        if (ADSB_MESSAGE[i] == ':'){
                            std::cin >> ICAOlat;
                            if(ICAOlat!=0){
                                latFound = Found;
                            }
                            break;
                        }
                    }
                    else break;
                }
                for(int i=0;i<50&&!lonFound;i++){
                    if (ADSB_MESSAGE[i] == ICAO_LON[i]){
                        if (ADSB_MESSAGE[i] == ':'){
                            std::cin >> ICAOlon;
                            if(ICAOlon!=0){
                                lonFound = Found;
                            }
                            break;
                        }
                    }
                    else break;
                }
                ICAOsquak=-1;
                for(int i=0;i<50;i++){
                    if (ADSB_MESSAGE[i] == ICAO_SQK[i]){
                        if (ADSB_MESSAGE[i] == ':'){
                            std::cin >> ICAOsquak;
                            break;
                        }
                    }
                    else break;
                }
                ICAOheading=-1;
                for(int i=0;i<50;i++){
                    if (ADSB_MESSAGE[i] == ICAO_HDG[i]){
                        if (ADSB_MESSAGE[i] == ':'){
                            std::cin >> ICAOheading;
                            break;
                        }
                    }
                    else break;
                }
                ICAOspeed=-1;
                for(int i=0;i<50;i++){
                    if (ADSB_MESSAGE[i] == ICAO_SPD[i]){
                        if (ADSB_MESSAGE[i] == ':'){
                            std::cin >> ICAOspeed;
                            break;
                        }
                    }
                    else break;
                }
                /** Now put the found information into the database. **/
                bool nameFound = NotFound;
                /** Search for same airplane or for a sleeping airplane. **/
                for(int i=0;i<MaxAircraft;i++){
                    /** Search name **/
                    nameFound = Found;
                    for(int n=0;n<20;n++){
                        if(O_ADSB_Database[i].ADSB_Address[n]!=ICAOadr[n]){
                            nameFound = NotFound;
                            break;
                        }
                    }
                    /** If name is the same then we update it's location **/
                    if(nameFound==Found){
                        //Update Lat&Long
                        if(lonFound && latFound){
                            O_ADSB_Database[i].CALC_Ydistance=O_ADSB_Database[i].ADSB_Ydistance=(ICAOlat - homeLat)*59.71922; //convert to nautical
                            float ABShomeLat = homeLat; if(ABShomeLat<0)ABShomeLat*=-1;  //get absolute value of homeLat
                            O_ADSB_Database[i].CALC_Xdistance=O_ADSB_Database[i].ADSB_Xdistance=(cos(ABShomeLat)*60.09719)*(ICAOlon-homeLon);
                            O_ADSB_Database[i].ADSB_preXY=1;
                            O_ADSB_Database[i].CALC_timer = interpCycles;
                            O_ADSB_Database[i].AircraftAsleepTimer = SleepTimer; //Reset it's sleep timer.
                        }
                        //Update Squawk
                        if(ICAOsquak>0)O_ADSB_Database[i].ADSB_Squawk = ICAOsquak;
                        //Update Heading
                        if(ICAOheading>-1)O_ADSB_Database[i].ADSB_HDG = ICAOheading;
                        //Update peed
                        if(ICAOspeed>-1)O_ADSB_Database[i].ADSB_SPD = ICAOspeed;
                        //If we get an update then reset the timer.
                        //But only if we have already gotten at least one geo-cord + heading + speed data.
                        if(O_ADSB_Database[i].ADSB_preXY&&O_ADSB_Database[i].ADSB_SPD>-1&&O_ADSB_Database[i].ADSB_HDG>-1)
                            O_ADSB_Database[i].AircraftAsleepTimer = SleepTimer;
                        //If out of range then remove from list via setting SleepTimer to Asleep;
                        if(ABSfloat(O_ADSB_Database[i].CALC_Ydistance)>MaxRangeY
                           || ABSfloat(O_ADSB_Database[i].CALC_Xdistance)>MaxRangeX)O_ADSB_Database[i].AircraftAsleepTimer = Sleeping;
                        break;
                    }
                }
                /** If name not found, search for a sleeping plane to replace or update **/
                if(nameFound==NotFound){
                    for(int i=0;i<MaxAircraft;i++){
                        if(O_ADSB_Database[i].AircraftAsleepTimer == Sleeping){
                            for(int n=0;n<20;n++){
                                O_ADSB_Database[i].ADSB_Address[n]=ICAOadr[n];  //Update the address/name.
                            }
                            if(lonFound && latFound){
                                O_ADSB_Database[i].CALC_Ydistance=O_ADSB_Database[i].ADSB_Ydistance = (ICAOlat - homeLat)*59.71922; //convert to nautical
                                if(ABSfloat(O_ADSB_Database[i].ADSB_Ydistance) > MaxRangeY)break;
                                float ABShomeLat = homeLat; if(ABShomeLat<0)ABShomeLat*=-1;  //get absolute value of homeLat
                                O_ADSB_Database[i].CALC_Xdistance=O_ADSB_Database[i].ADSB_Xdistance = (cos(ABShomeLat)*60.09719)*(ICAOlon-homeLon);
                                if(ABSfloat(O_ADSB_Database[i].ADSB_Xdistance) > MaxRangeX)break;
                                O_ADSB_Database[i].ADSB_preXY=1;
                                O_ADSB_Database[i].CALC_timer = interpCycles;
                                O_ADSB_Database[i].AircraftAsleepTimer = SleepTimer; //Reset it's sleep timer.
                            }
                            /** If it's a new aircraft and we aren't getting geo-data then Reset Previous XY get. **/
                            else O_ADSB_Database[i].ADSB_preXY=0;
                            /** Update Squawk Code **/
                            if(ICAOsquak!=0)O_ADSB_Database[i].ADSB_Squawk = ICAOsquak;
                            /** If it's a new name and not getting a squawk code then reset it to 0 **/
                            else O_ADSB_Database[i].ADSB_Squawk = -1;
                            //Update Heading
                            if(ICAOheading!=0)O_ADSB_Database[i].ADSB_HDG = ICAOheading;
                            else O_ADSB_Database[i].ADSB_HDG = -1;
                            //Update Speed
                            if(ICAOspeed!=0)O_ADSB_Database[i].ADSB_SPD = ICAOspeed;
                            else O_ADSB_Database[i].ADSB_SPD = -1;
                            break;
                        }
                    }
                }
           }
    }
    std::cout << "ADS-B_Getter thread terminated. \n";
}

void F_ADSBpred(){
    while(ADSBpRun){
        for(int i=0;i<MaxAircraft;i++){
            if(O_ADSB_Database[i].AircraftAsleepTimer!=Sleeping
                &&O_ADSB_Database[i].ADSB_preXY
                &&O_ADSB_Database[i].ADSB_SPD>-1
                &&O_ADSB_Database[i].ADSB_HDG>-1){
                    if(O_ADSB_Database[i].CALC_timer<=0){
                        O_ADSB_Database[i].CALC_timer=interpCycles;
                        O_ADSB_Database[i].CALC_Xdistance = O_ADSB_Database[i].CALC_Xdistance+((std::sin(DegToRad(O_ADSB_Database[i].ADSB_HDG))*(O_ADSB_Database[i].ADSB_SPD/interpFactor)));
                        O_ADSB_Database[i].CALC_Ydistance = O_ADSB_Database[i].CALC_Ydistance+((std::cos(DegToRad(O_ADSB_Database[i].ADSB_HDG))*(O_ADSB_Database[i].ADSB_SPD/interpFactor)));
                        //If out of range then remove from list via setting SleepTimer to Asleep;
                        if(ABSfloat(O_ADSB_Database[i].CALC_Ydistance)>MaxRangeY
                           || ABSfloat(O_ADSB_Database[i].CALC_Xdistance)>MaxRangeX)O_ADSB_Database[i].AircraftAsleepTimer = Sleeping;

                        //O_ADSB_Database[i].timeEnd = std::chrono::high_resolution_clock::now();
                        //double timeFactor = std::chrono::duration_cast<std::chrono::nanoseconds>(O_ADSB_Database[i].timeEnd-O_ADSB_Database[i].timeBegin).count()
                        //O_ADSB_Database[i].timeBegin = std::chrono::high_resolution_clock::now();
                    }
                    else O_ADSB_Database[i].CALC_timer--;
                }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(interpTime));
    }
    std::cout << "ADS-B_Predictor thread terminated. \n";
}

float ABSfloat(float in){
    float i=0;
    if(in<0)i=in*-1;
    else i=in;
    return i;
}

float DegToRad(int in){
    return (in*PI/180);
}

