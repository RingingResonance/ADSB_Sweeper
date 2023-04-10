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
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>



bool runDACscope = 0;
float blipScale = 1;
int blankInten = 1023;
int dimInten = 512;
int scaleInten = 470;
int blipInten = 0;
bool bufferSel = 0;
bool IntenThrdsRun = 1;
int procID=0;

uint16_t Xout[XYpreCalcSweep];
uint16_t Yout[XYpreCalcSweep];
uint8_t SPI_OUT[SPI_OUT_Cnt];
unsigned char INTEN_OUT[SPI_INT_Cnt][2];
unsigned char XYout[XYdataCnt];
unsigned char Intenout[msgCnt];

float distFactRecip;
float distFactor = 128.0f;
float blipy = 0.035f;
float blipy_sq = 0.0f;

inline static float sq(float x) { return x*x; }
C_bulkSweepCalc* O_bulkSweepCalc;


/** DAC Scope Loop **/
int F_DACscope() {
    O_bulkSweepCalc = new C_bulkSweepCalc [DACscopeThreadCnt];
  if(!runDACscope) return 0;
  std::cout << "Starting DAC Scope Thread. \n";
  /** Open and configure SPI interface **/
  uint8_t OutMode = SPI_MODE_0;
  uint8_t OutBits = 8;
  uint8_t Default = 0;
  uint8_t TwoBytes = 2;
  uint32_t OutSpeed = 32000000;
  distFactRecip = 1.0f / distFactor;
  blipy = (512/distFactor)*(blipScale*0.00875);
  blipy_sq = blipy * blipy;
  auto XY_SPI = open("/dev/spidev0.0", O_RDWR);
  auto INTEN_SPI = open("/dev/spidev0.1", O_RDWR);
  ///Configure the XY output.
  if (XY_SPI < 0) {
    runDACscope = 0;
    std::cout << "Cannot open /dev/spidev0.0 errno: " << errno << " \n";
  } else {
    if (ioctl(XY_SPI, SPI_IOC_WR_MODE, & OutMode) < 0) {
      runDACscope = 0;
      std::cout << "Error 1, cannot set Write mode. errno: " << errno << " \n";
    }
    if (ioctl(XY_SPI, SPI_IOC_WR_MAX_SPEED_HZ, & OutSpeed) < 0) {
      std::cout << "Error 2, cannot set speed. errno: " << errno << "\n";
      runDACscope = 0;
    }
    if (ioctl(XY_SPI, SPI_IOC_WR_BITS_PER_WORD, & OutBits) < 0) {
      std::cout << "Error 3, cannot set word size. errno: " << errno << " \n";
      runDACscope = 0;
    }
    if (runDACscope) std::cout << "Successfully opened and configured /dev/spidev0.0 \n";
  }
  ///Configure the Intensity output.
  if (INTEN_SPI < 0) {
    runDACscope = 0;
    std::cout << "Cannot open /dev/spidev0.1 errno: " << errno << " \n";
  } else {
    if (ioctl(INTEN_SPI, SPI_IOC_WR_MODE, & OutMode) < 0) {
      runDACscope = 0;
      std::cout << "Error 1, cannot set Write mode. errno: " << errno << " \n";
    }
    if (ioctl(INTEN_SPI, SPI_IOC_WR_MAX_SPEED_HZ, & OutSpeed) < 0) {
      std::cout << "Error 2, cannot set speed. errno: " << errno << "\n";
      runDACscope = 0;
    }
    if (ioctl(INTEN_SPI, SPI_IOC_WR_BITS_PER_WORD, & OutBits) < 0) {
      std::cout << "Error 3, cannot set word size. errno: " << errno << " \n";
      runDACscope = 0;
    }
    if (runDACscope) std::cout << "Successfully opened and configured /dev/spidev0.1 \n";
  }
  std::cout << "Setting up memory for SPI output table. \n";
  struct spi_ioc_transfer XYmesg[msgCnt*2];
  struct spi_ioc_transfer INTENmesg[SyncCnt];
  memset( &XYmesg, 0, sizeof(XYmesg));
  memset( &INTENmesg, 0, sizeof(INTENmesg));
  std::cout << "Generating initial SPI output tables. \n";
  ///Need to send 16 bits but the Raspi's hardware only supports 8 bits.
  ///Do XY
  int iInt = 0;
  bool iIntINC = 0;
  ///Organize the pointers to alternate between XY and Intensity data.
  for (int i = 0; i < msgCnt*2; i++) {
    XYmesg[i].rx_buf = (unsigned long) nullptr;
    XYmesg[i].bits_per_word = 8; // 0;
    XYmesg[i].tx_nbits = 0; // 0;
    XYmesg[i].pad = 0; // 0;
    XYmesg[i].word_delay_usecs = 0; // 0;
    XYmesg[i].speed_hz = 12000000;
    XYmesg[i].len = 2;
    XYmesg[i].delay_usecs = 0; // 0
    XYmesg[i].cs_change = 1; /// Bring CS high between messages. Right? Is this how this god-forsaken driver works? IDK. So much conflicting info on the net...
    if(i&0x0001){
        XYmesg[i].tx_buf = (unsigned long) &XYout[iInt*2];
        if(iIntINC){iInt++; iIntINC=0;}
        else iIntINC=1;
    }
    else {
        XYmesg[i].tx_buf = (unsigned long) &Intenout[i];
    }
  }
  ///Do sync pulse. Using CE1 for this.
  for (int i = 0; i < SyncCnt; i++) {
    INTENmesg[i].rx_buf = (unsigned long) nullptr;
    INTENmesg[i].bits_per_word = 8; // 0;
    INTENmesg[i].tx_nbits = 0; // 0;
    INTENmesg[i].word_delay_usecs = 0; // 0;
    INTENmesg[i].pad = 0; // 0;
    INTENmesg[i].speed_hz = 32000000;
    INTENmesg[i].len = 0;
    INTENmesg[i].delay_usecs = 0; // 0
    INTENmesg[i].cs_change = 0; // 0;
    INTENmesg[i].tx_buf = (unsigned long) nullptr;
  }
  /******************************************/
    PreCalcSweep();   ///Pre-Calc XY position data.
    ///Create processing threads.
    std::cout << "Creating processing threads. \n";
    procID=0;
    std::thread IntensityThread1(O_bulkSweepCalc[0].ScopeCalc);
    usleep(100);
    procID++;
    std::thread IntensityThread2(O_bulkSweepCalc[1].ScopeCalc);
    std::cout << "DACscope Running. \n";
    while (runDACscope) {
        /// Dispatch data to the DACs if data is ready.
        if(O_bulkSweepCalc[0].dataReady && O_bulkSweepCalc[1].dataReady){
            if(bufferSel)bufferSel=0; else bufferSel=1; ///Change write-to buffer.
            for(int i=0;i<DACscopeThreadCnt;i++){O_bulkSweepCalc[i].dataReady=0;}   ///Get the threads running again.
            ///Output the data.
            for(int i=0;i<bulkTransFactor;i++){
                ///Send any data out /dev/spidev0.1 to sync the external CS logic.
                if (ioctl(INTEN_SPI, SPI_IOC_MESSAGE(SyncCnt), & INTENmesg) < 0) {
                    std::cout << "Sending data to /dev/spidev0.1 failed. errno: " << errno << " \n";
                    runDACscope = 0; //Kill the display if write failure.
                    break;
                }

                ///Transfer XY and Intensity data.
                for(int d=0;d<msgCnt;d++){Intenout[d]=INTEN_OUT[d+(i*msgCnt)][!bufferSel];}

                for(int d=0;d<XYdataCnt;d++){XYout[d]=SPI_OUT[d+(i*XYdataCnt)];}

                if (ioctl(XY_SPI, SPI_IOC_MESSAGE(msgCnt*2), &XYmesg) < 0) {
                    std::cout << "Sending data to /dev/spidev0.0 failed. errno: " << errno << " \n";
                    runDACscope = 0; //Kill the display if write failure.
                    break;
                }
                if(!runDACscope)break;
            }
        }
    }
  close(XY_SPI);
  IntenThrdsRun=0;
  //IntensityThread1.join;
  //IntensityThread2.join;
  std::cout << "DAC Scope thread terminated. \n";
  return 0;
}

int C_bulkSweepCalc::ScopeCalc(void) {
    int myID = procID;
    int SWP_LowLimit = (XYpreCalcSweep/DACscopeThreadCnt)*myID;
    int SWP_HighLimit = (XYpreCalcSweep/DACscopeThreadCnt)*(myID+1);
    while(IntenThrdsRun){
        if(!O_bulkSweepCalc[myID].dataReady){
            int INTENindex = (SPI_INT_Cnt/DACscopeThreadCnt)*myID;
            ///Calculate Intensity data.
            uint16_t Bright;
            for(int pcIndex = SWP_LowLimit; pcIndex<SWP_HighLimit;pcIndex++){
                int radarTrace = ((pcIndex*XYresDivider) & 0x03FF)-512;
                if (radarTrace>2) {
                    ///Draw Scale.
                    if ((radarTrace&~0x003F)==radarTrace){
                        Bright = scaleInten; ///Give a little more/less brightness.
                    }
                    else {
                        Bright = dimInten; ///Give some brightness.
                    }

                    float XaComp = Xout[pcIndex];
                    float YaComp = Yout[pcIndex];
                    float XdComp = (XaComp - XYresHighLim) * (distFactRecip);
                    float YdComp = (YaComp - XYresHighLim) * (distFactRecip);
                    for ( int b=0; b<MaxAircraft; ++b ) {
                        if (O_ADSB_Database[b].AircraftAsleepTimer) {
                            ///Get distance from point of measurement to aircrafts.
                            float dist_sq =
                                sq( XdComp + O_ADSB_Database[b].CALC_Xdistance ) +
                                sq( YdComp - O_ADSB_Database[b].CALC_Ydistance );
                            if ( dist_sq < blipy_sq ) {
                                Bright = blipInten; ///We got a ping! Full brightness!
                                break;
                }}}}
                else {
                    Bright = blankInten; ///All the way dark.
                }
                uint16_t Intensity = ((Bright << 2) & 0x0FFF) | 0xF000; ///Update A&B with same data and latch.
                INTEN_OUT[INTENindex    ][bufferSel] = (Intensity >> 8);
                INTEN_OUT[INTENindex + 1][bufferSel] =  Intensity;
                INTENindex += 2;
            }
            O_bulkSweepCalc[myID].dataReady=1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0;
}

///Function for pre-calculating XY position data for an entire sweep.
void PreCalcSweep(void){
    std::cout << "Generating XY data. \n";
///Sweep
    unsigned int XYindex = 0;
    for (int radarSweep = 0; radarSweep < DEGsteps; radarSweep++) {
        ///Trace
        float rad = DegToRad(radarSweep);
        float cosCalc = std::cos(rad);
        float sinCalc = std::sin(rad);
        for (int radarTrace = XYresLowLim; radarTrace < XYresHighLim; radarTrace+=XYresDivider) {
            ///Calculate X sweep data.
            uint16_t XUout = Xout[XYindex] = (cosCalc * radarTrace) + XYresHighLim;
            uint16_t XTout = ((XUout << 2) & 0x0FFF) | 0x1000; ///Update A, but do not latch.
            SPI_OUT[(XYindex*4)  ] = (XTout >> 8);
            SPI_OUT[(XYindex*4)+1] =  XTout;
            ///Calculate Y sweep data.
            uint16_t YUout = Yout[XYindex] = (sinCalc * radarTrace) + XYresHighLim;
            uint16_t YTout = ((YUout << 2) & 0x0FFF) | 0xA000; ///Update B and latch both A and B outputs.
            SPI_OUT[(XYindex*4)+2] = (YTout >> 8);
            SPI_OUT[(XYindex*4)+3] =  YTout;
            XYindex++;
        }
    }
    std::cout << "XY data created. \n";
}
