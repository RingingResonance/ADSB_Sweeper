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

#ifndef ADSBdata_H
#define ADSBdata_H


#define NotFound 0
#define Found 1
#define PI 3.14159265

#define interpFactor 28800
#define interpCycles 1000
#define interpTime 125
#define interpCyclesPerSec 1000000/interpTime
#define Sleeping 0
#define Awake 1

extern double  homeLon;
extern double  homeLat;

extern int MaxAvrgCnt;
extern double  avrgLon;
extern double  avrgLat;
extern int     LocAvrgCnt;

extern float MaxRange;

/** Strings of text to look for from dump1090 **/
static char ICAO_ADR[] = {"Address:  "};
static char ICAO_LAT[] = {"latitude:  "};
static char ICAO_LON[] = {"longitude:  "};
static char ICAO_SQK[] = ("Squawk:  ");
static char ICAO_HDG[] = ("Heading:  ");
static char ICAO_SPD[] = ("Speed:  ");
extern int  sweeptimer;

extern bool addressFound;
extern bool ADSBgRun;
extern bool ADSBpRun;
extern bool runDACscope;
extern bool runCLIscope;
extern bool latFound;
extern bool lonFound;

extern int SleepTimer;
extern int MaxAircraft;

float ABSfloat(float);
void F_ADSBgetter(void);
void F_ADSBpred(void);
float DegToRad(float);

class C_ADSB_Database{
public:
    int timeBegin;
    int timeEnd;
    char * ADSB_Address;
    int  ADSB_Squawk = 0;
    int sleepMult=0;
    float  ADSB_HDG = 0;
    float  ADSB_SPD = 0;
    float ADSB_Xdistance = 0;
    float ADSB_Ydistance = 0;
    int CALC_timer = interpFactor;
    float  CALC_HDG = 0;
    float  CALC_SPD = 0;
    double CALC_Xdistance = 0;
    double CALC_Ydistance = 0;
    bool ADSB_preXY=0;
    int AircraftAsleepTimer = 0;
    /** Memory constructor **/
    C_ADSB_Database(void){
        ADSB_Address = new char [20];
        for(int i=0;i<20;i++) ADSB_Address[i]=0;
    }
    ~C_ADSB_Database(void){
        delete[] ADSB_Address;
    }
};extern C_ADSB_Database* O_ADSB_Database;


#endif // PICTOVECTWAVE_H
