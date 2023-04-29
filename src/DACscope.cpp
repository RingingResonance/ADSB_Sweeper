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
bool InBufferSel = 0;
bool OutBufRdy[2] = {0,0};
bool OutBufSel = 0;
int procID=0;
int XY_SPI = 0;
int INTEN_SPI = 0;
int SPIfreq = 5000000;
int SPIdly = 2;
bool calcFirstRun=0;
int DegRot = 0;

uint16_t Xout[XYpreCalcSweep*DEGsteps];
uint16_t Yout[XYpreCalcSweep*DEGsteps];
uint8_t SPI_OUT[SPI_OUT_Cnt];
unsigned char INTEN_OUT0[SPI_INT_Cnt];
unsigned char INTEN_OUT1[SPI_INT_Cnt];

struct spi_ioc_transfer XYmesg0[msgCnt];
struct spi_ioc_transfer XYmesg1[msgCnt];
struct spi_ioc_transfer INTENmesg[SyncCnt];

float distFactRecip;
///Default values.
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
  XY_SPI = open("/dev/spidev0.0", O_RDWR);
  INTEN_SPI = open("/dev/spidev0.1", O_RDWR);
  ///Configure the XY output.
  if (XY_SPI < 0) {
    runDACscope = 0;
    std::cout << "**Cannot open /dev/spidev0.0 errno: " << errno << " \n";
  } else {
    if (ioctl(XY_SPI, SPI_IOC_WR_MODE, & OutMode) < 0) {
      runDACscope = 0;
      std::cout << "**Error 1, cannot set Write mode. errno: " << errno << " \n";
    }
    if (ioctl(XY_SPI, SPI_IOC_WR_MAX_SPEED_HZ, & OutSpeed) < 0) {
      std::cout << "**Error 2, cannot set speed. errno: " << errno << "\n";
      runDACscope = 0;
    }
    if (ioctl(XY_SPI, SPI_IOC_WR_BITS_PER_WORD, & OutBits) < 0) {
      std::cout << "**Error 3, cannot set word size. errno: " << errno << " \n";
      runDACscope = 0;
    }
    if (runDACscope) std::cout << "Successfully opened and configured /dev/spidev0.0 \n";
  }
  ///Configure the Intensity output.
  if (INTEN_SPI < 0) {
    runDACscope = 0;
    std::cout << "**Cannot open /dev/spidev0.1 errno: " << errno << " \n";
  } else {
    if (ioctl(INTEN_SPI, SPI_IOC_WR_MODE, & OutMode) < 0) {
      runDACscope = 0;
      std::cout << "**Error 1, cannot set Write mode. errno: " << errno << " \n";
    }
    if (ioctl(INTEN_SPI, SPI_IOC_WR_MAX_SPEED_HZ, & OutSpeed) < 0) {
      std::cout << "**Error 2, cannot set speed. errno: " << errno << "\n";
      runDACscope = 0;
    }
    if (ioctl(INTEN_SPI, SPI_IOC_WR_BITS_PER_WORD, & OutBits) < 0) {
      std::cout << "**Error 3, cannot set word size. errno: " << errno << " \n";
      runDACscope = 0;
    }
    if (runDACscope) std::cout << "Successfully opened and configured /dev/spidev0.1 \n";
  }
  std::cout << "Setting up memory for SPI output table. \n";
  memset( &XYmesg0, 0, sizeof(XYmesg0));
  memset( &XYmesg1, 0, sizeof(XYmesg1));
  memset( &INTENmesg, 0, sizeof(INTENmesg));
  std::cout << "Generating initial SPI output tables. \n";
  ///Need to send 16 bits but the Raspi's hardware only supports 8 bits.
  for (int i = 0; i < msgCnt; i++) {
    XYmesg0[i].rx_buf = (unsigned long) nullptr;
    XYmesg0[i].bits_per_word = 8; // 0;
    XYmesg0[i].tx_nbits = 0; // 0;
    XYmesg0[i].pad = 0; // 0;
    XYmesg0[i].word_delay_usecs = 0; // 0;
    XYmesg0[i].speed_hz = SPIfreq;
    XYmesg0[i].len = 2;
    XYmesg0[i].delay_usecs = SPIdly; // 0
    XYmesg0[i].cs_change = 1; /// Bring CS high between messages. Right? Is this how this god-forsaken driver works? IDK. So much conflicting info on the net...
  }
  for (int i = 0; i < msgCnt; i++) {
    XYmesg1[i].rx_buf = (unsigned long) nullptr;
    XYmesg1[i].bits_per_word = 8; // 0;
    XYmesg1[i].tx_nbits = 0; // 0;
    XYmesg1[i].pad = 0; // 0;
    XYmesg1[i].word_delay_usecs = 0; // 0;
    XYmesg1[i].speed_hz = SPIfreq;
    XYmesg1[i].len = 2;
    XYmesg1[i].delay_usecs = SPIdly; // 0
    XYmesg1[i].cs_change = 1; /// Bring CS high between messages. Right? Is this how this god-forsaken driver works? IDK. So much conflicting info on the net...
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
    ///Start output buffer transfer thread.
    std::thread SPIoutThread(OutBufferSend);
    std::cout << "DACscope Running. \n";
    int DegRotOut = 0;
    while (runDACscope) {
        /// Dispatch data to the DACs if data is ready.
        if(O_bulkSweepCalc[0].dataReady && O_bulkSweepCalc[1].dataReady){
            if(DegRot<360)DegRot++; else DegRot=0;
            if(InBufferSel)InBufferSel=0; else InBufferSel=1; ///Change write-to buffer.
            for(int i=0;i<DACscopeThreadCnt;i++){O_bulkSweepCalc[i].dataReady=0;}   ///Get the threads running again.
            DegRotOut=DegRot-1;
            if(DegRotOut==-1)DegRotOut=359;
            ///Output the data.
            for(int i=0;i<bulkTransFactor;i++){
                ///If both buffers are full then loop here.
                while(OutBufRdy[0]&&OutBufRdy[1]){std::this_thread::sleep_for(std::chrono::microseconds(1));}///Do nothing but loop.
                ///Fill one of the buffers.
                ///If a buffer is ready, select the other one instead.
                int iXY = 0;
                if(!OutBufRdy[0]){
                    OutBufSel=0; ///Writing pointers for output Buffer 0
                    int adrFact1 = (i*msgCnt)+(DegRotOut*DACresolution);
                    int adrFact2 = (i*msgCnt)>>1;
                    if(InBufferSel){
                        ///Write from calculated data buffer 1
                        for(int d=0;d<msgCnt;d+=2) {
                            XYmesg0[d].tx_buf = (unsigned long)&INTEN_OUT1[(iXY&0xFE)+adrFact2];
                            XYmesg0[d+1].tx_buf = (unsigned long)&SPI_OUT [(iXY<<1)+adrFact1];
                            iXY++;
                        }
                    }
                    else {
                        ///Write from calculated data buffer 0
                        for(int d=0;d<msgCnt;d+=2) {
                            XYmesg0[d].tx_buf = (unsigned long)&INTEN_OUT0[(iXY&0xFE)+adrFact2];
                            XYmesg0[d+1].tx_buf = (unsigned long)&SPI_OUT [(iXY<<1)+adrFact1];
                            iXY++;
                        }
                    }
                }
                else {
                    OutBufSel=1; ///Writing pointers to output buffer 1
                    int adrFact1 = (i*msgCnt)+(DegRotOut*DACresolution);
                    int adrFact2 = (i*msgCnt)>>1;
                    if(InBufferSel){
                        ///Write from calculated data buffer 1
                        for(int d=0;d<msgCnt;d+=2) {
                            XYmesg1[d].tx_buf = (unsigned long)&INTEN_OUT1[(iXY&0xFE)+adrFact2];
                            XYmesg1[d+1].tx_buf = (unsigned long)&SPI_OUT [(iXY<<1)+adrFact1];
                            iXY++;
                        }
                    }
                    else {
                        ///Write from calculated data buffer 0
                        for(int d=0;d<msgCnt;d+=2) {
                            XYmesg1[d].tx_buf = (unsigned long)&INTEN_OUT0[(iXY&0xFE)+adrFact2];
                            XYmesg1[d+1].tx_buf = (unsigned long)&SPI_OUT [(iXY<<1)+adrFact1];
                            iXY++;
                        }
                    }
                }
                OutBufRdy[OutBufSel]=1;
                if(!runDACscope)break;
            }
        }
    }
  close(XY_SPI);
  SPIoutThread.join();
  IntensityThread1.join();
  IntensityThread2.join();
  ADSBpRun=0;
  ADSBgRun=0;
  runCLIscope=0;
  std::cout << "DAC Scope thread terminated. \n";
  return 0;
}

///Output buffer send thread.
int OutBufferSend(void){
    std::cout << "Starting DAC SPI output thread. \n";
    while(runDACscope){
        ///If neither are ready, loop until one is ready.
        if(!OutBufRdy[0]&&!OutBufRdy[1]&&calcFirstRun)std::cout << "!!->Can't keep up with requested SPI speed.<-!! \n";
        while(!OutBufRdy[0]&&!OutBufRdy[1]){}   ///Do nothing but loop.
        ///Send any data out /dev/spidev0.1 to sync the external CS logic.
        if (ioctl(INTEN_SPI, SPI_IOC_MESSAGE(SyncCnt), &INTENmesg) < 0) {
            std::cout << "**Sending data to /dev/spidev0.1 failed. errno: " << errno << " \n";
            runDACscope = 0; //Kill the display if write failure.
            break;
        }
        if(OutBufRdy[0] && OutBufSel){
            ///Transfer XY and Intensity data.
            ///Transfer to Buffer 1 and output Buffer 0.
            if (ioctl(XY_SPI, SPI_IOC_MESSAGE(msgCnt), &XYmesg0) < 0) {
                std::cout << "**Sending data to /dev/spidev0.0 failed. errno: " << errno << " \n";
                runDACscope = 0; //Kill the display if write failure.
                break;
            }
            OutBufRdy[0]=0;    ///Set this buffer as empty.
        }
        else if(OutBufRdy[1] && !OutBufSel){
            ///Transfer to Buffer 0 and output Buffer 1.
            if (ioctl(XY_SPI, SPI_IOC_MESSAGE(msgCnt), &XYmesg1) < 0) {
                std::cout << "**Sending data to /dev/spidev0.0 failed. errno: " << errno << " \n";
                runDACscope = 0; //Kill the display if write failure.
                break;
            }
            OutBufRdy[1]=0;    ///Set this buffer as empty.
        }
    }
    std::cout << "# DAC SPI thread terminated. \n";
    return 0;
}

///Intensity calc thread(s).
int C_bulkSweepCalc::ScopeCalc(void) {
    std::cout << "Starting DAC Intensity thread. \n";
    int myID = procID;
    int SWP_LowLimit = (XYpreCalcSweep/DACscopeThreadCnt)*myID;
    int SWP_HighLimit = (XYpreCalcSweep/DACscopeThreadCnt)*(myID+1);
    while(runDACscope){
        if(!O_bulkSweepCalc[myID].dataReady){
            int INTENindex = (SPI_INT_Cnt/DACscopeThreadCnt)*myID;
            ///Calculate Intensity data.
            uint16_t Bright;
            for(int pcIndex = SWP_LowLimit; pcIndex<SWP_HighLimit;pcIndex++){
                ///If neither output buffers are ready then pause these threads for a moment.
                if(!OutBufRdy[0]&&!OutBufRdy[1]&&calcFirstRun)std::this_thread::sleep_for(std::chrono::microseconds(1));
                int radarTrace = ((pcIndex*XYresDivider) & 0x03FF)-512;
                if (radarTrace>2) {
                    ///Draw Scale.
                    if ((radarTrace&~0x003F)==radarTrace){
                        Bright = scaleInten; ///Give a little more/less brightness.
                    }
                    else {
                        Bright = dimInten; ///Give some brightness.
                    }
                    ///Check for aircraft.
                    float XaComp = Xout[pcIndex+(DegRot*XYpreCalcSweep)];
                    float YaComp = Yout[pcIndex+(DegRot*XYpreCalcSweep)];
                    float XdComp = (XaComp - XYresHighLim) * (distFactRecip);
                    float YdComp = (YaComp - XYresHighLim) * (distFactRecip);
                    for ( int b=0; b<MaxAircraft; ++b ) {
                        if (O_ADSB_Database[b].AircraftAsleepTimer) {
                            ///Get distance from point of measurement to aircrafts.
                            float dist_sq =
                                sq( XdComp + O_ADSB_Database[b].CALC_Xdistance ) +
                                sq( YdComp - O_ADSB_Database[b].CALC_Ydistance );
                            ///If distance is within range, make a blip on the scope.
                            if ( dist_sq < blipy_sq ) {
                                Bright = blipInten; ///We got a ping! Full brightness!
                                break;
                }}}}
                else {
                    Bright = blankInten; ///All the way dark.
                }
                uint16_t Intensity = ((Bright << 2) & 0x0FFF) | 0xF000; ///Update A&B with same data and latch.
                if(!InBufferSel){
                    INTEN_OUT0[INTENindex    ] = (Intensity >> 8);
                    INTEN_OUT0[INTENindex + 1] =  Intensity;
                }
                else{
                    INTEN_OUT1[INTENindex    ] = (Intensity >> 8);
                    INTEN_OUT1[INTENindex + 1] =  Intensity;
                }
                INTENindex += 2;
            }
            O_bulkSweepCalc[myID].dataReady=1;
            calcFirstRun=1;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    std::cout << "# DAC Intensity thread terminated. \n";
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
