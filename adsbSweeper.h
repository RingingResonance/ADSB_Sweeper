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

#ifndef asdbSweeper_H
#define asdbSweeper_H

#define sweepTime 2
#define zoomFactorX 10
#define zoomFactorY 5
#define MaxRangeX 4
#define MaxRangeY 4

#define NotFound 0
#define Found 1
#define PI 3.14159265

#define interpFactor 28800
#define interpCycles 1000
#define interpTime 125
#define SleepTimer 100
#define MaxAircraft 26
#define Sleeping 0
#define Awake 1

#define homeLon -97.088857
#define homeLat 32.831865

///Default max msgCnt size seems to be 128
#define msgCnt 8        ///8 seems to work with 1024 resolution. Nothing else works and I'm not sure why.
#define intCnt 4
#define SPI_OUT_Cnt 32
#define SPI_INT_Cnt 16
unsigned char SPI_OUT[SPI_OUT_Cnt];
unsigned char INTEN_OUT[SPI_INT_Cnt];
int XYindex = 0;
int INTENindex = 0;
int Intensity = 0;
int Bright = 0;
int dspchCnt = 0;
int radarSweep = 0;
int radarTrace = 0;

/** Strings of text to look for from dump1090 **/
char DSP_ID[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
char ICAO_ADR[] = {"Address:  "};
char ICAO_LAT[] = {"latitude:  "};
char ICAO_LON[] = {"longitude:  "};
char ICAO_SQK[] = ("Squawk:  ");
char ICAO_HDG[] = ("Heading:  ");
char ICAO_SPD[] = ("Speed:  ");
int  sweeptimer = 0;

bool addressFound = NotFound;
bool ADSBgRun = 1;
bool ADSBpRun = 1;
bool displayRun = 1;
bool latFound = NotFound;
bool lonFound = NotFound;
float ABSfloat(float);
float DegToRad(float);
void F_Display(void);
void F_ADSBgetter(void);
void F_ADSBpred(void);

class C_ADSB_Database{
public:
    int timeBegin;
    int timeEnd;
    char * ADSB_Address;
    int  ADSB_Squawk = 0;
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
};C_ADSB_Database* O_ADSB_Database;


#endif // PICTOVECTWAVE_H
