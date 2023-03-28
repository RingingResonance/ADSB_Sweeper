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
    O_ADSB_Database = new C_ADSB_Database [200]; //Max 200 aircraft for now.
    /** Create loop for ADS-B information getter. **/
    pthread_t O_ADSBgetter;
    pthread_create(&O_ADSBgetter, NULL, F_ADSBgetter, NULL);
    /***********************************************/
    /** Calculate delay used for the sweep timer **/
    slptm.tv_sec = 0;
    float sweepTemp = 100000000*(1/(373760/sweepTime));
    sweeptimer = sweepTemp;
    slptm.tv_nsec = 1;
    for(;;){
        for(int radarSweep=0;radarSweep<365;radarSweep++){
            for(int radarTrace=-512;radarTrace<512;radarTrace++){
                int Xout = 512+(std::cos(radarSweep)*radarTrace);
                int Yout = 512+(std::sin(radarSweep)*radarTrace);
                //gpiod_line_set_value_bulk(GPIOout,0);
            }
        }
        std::cout << "\x1B[2J\x1B[H";
        char airplane[3000];
        char border[100];
        for(int i=0;i<100;i++)border[i]='#';border[99]=0;
        for(int i=0;i<3000;i++)airplane[i]=' ';
        for(int i=0;i<200;i++){
            if(O_ADSB_Database[i].AircraftAsleepTimer){
                    float tempX=(O_ADSB_Database[i].ADSB_Xdistance)+50;
                    float tempY=(O_ADSB_Database[i].ADSB_Ydistance*-1)+15;
                    int adrX=tempX;
                    int adrY=tempY;
                    if(adrX<0)adrX=0;if(adrX>99)adrX=99;
                    if(adrY<0)adrY=0;if(adrY>29)adrY=29;
                    airplane[(adrY*100)+adrX]='@';
            }
        }
        for(int x=0;x<30;x++){
                airplane[(x*100)+99]='#';
                airplane[(x*100)+100]='\n';
        }
        airplane[2999]=0;
        std::cout << airplane << "\n";
        std::cout << border << "\n";
        for(int i=0;i<200;i++){
            if(O_ADSB_Database[i].AircraftAsleepTimer){
                std::cout << O_ADSB_Database[i].ADSB_Address << "  "<< O_ADSB_Database[i].ADSB_Xdistance
                << "  " << O_ADSB_Database[i].ADSB_Ydistance << "\n";
                O_ADSB_Database[i].AircraftAsleepTimer--;
            }
        }
        sleep(1);
    }
    ADSBgRun = 0;
    pthread_join(O_ADSBgetter, NULL);
    return 0;
}

/** ADS-B loop **/
void *F_ADSBgetter(void *arg)
{
    char ADSB_MESSAGE[1000];
    char ICAOadr[20];
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
                /** Now put the found information into the database. **/
               if(lonFound && latFound){
                    int nameFound = NotFound;
                   /** Search for same airplane or for a sleeping airplane. **/
                    for(int i=0;i<200;i++){
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
                            O_ADSB_Database[i].AircraftAsleepTimer = SleepTimer; //Reset it's sleep timer.
                            O_ADSB_Database[i].ADSB_Ydistance = (ICAOlat - homeLat)*110.6; //convert to meters
                            float ABShomeLat = homeLat; if(ABShomeLat<0)ABShomeLat*=-1;  //get absolute value of homeLat
                            O_ADSB_Database[i].ADSB_Xdistance = (cos(ABShomeLat)*111.3)*(ICAOlon-homeLon);
                            break;
                        }
                    }
                    if(nameFound==NotFound){
                        /** Name not found, search for a sleeping plane to replace or update **/
                        for(int i=0;i<200;i++){
                            if(O_ADSB_Database[i].AircraftAsleepTimer == Sleeping){
                                for(int n=0;n<20;n++){
                                    O_ADSB_Database[i].ADSB_Address[n]=ICAOadr[n];  //Update the address/name.
                                }
                                O_ADSB_Database[i].AircraftAsleepTimer = SleepTimer; //Reset it's sleep timer.
                                O_ADSB_Database[i].ADSB_Ydistance = (ICAOlat - homeLat)*110.6; //convert to meters
                                float ABShomeLat = homeLat; if(ABShomeLat<0)ABShomeLat*=-1;  //get absolute value of homeLat
                                O_ADSB_Database[i].ADSB_Xdistance = (cos(ABShomeLat)*111.3)*(ICAOlon-homeLon);
                                break;
                            }
                        }
                    }
               }
           }
    }
    std::cout << "ADS-B_Getter thread terminated. \n";
    return 0;
}



