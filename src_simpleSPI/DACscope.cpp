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

unsigned char SPI_OUT[SPI_OUT_Cnt];
unsigned char INTEN_OUT[SPI_INT_Cnt];
int XYindex = 0;
int INTENindex = 0;
int dspchCnt = 0;
int radarSweep = 0;
int radarTrace = 0;
bool runDACscope = 0;
float blipScale = 1;
int blankInten = 1023;
int dimInten = 512;
int scaleInten = 470;
int blipInten = 0;

float distFactRecip;
float distFactor = 128.0f;
float blipy = 0.035f;
float blipy_sq = 0.0f;

inline static float sq(float x) { return x*x; }
void ScopeCalc(float cosCalc, float sinCalc) {
    ///Calculate X sweep data.
    uint16_t Xout = (cosCalc * radarTrace) + 512;
    uint16_t XTout = ((Xout << 2) & 0x0FFF) | 0x1000; ///Update A, but do not latch.
    SPI_OUT[XYindex    ] = (XTout >> 8);
    SPI_OUT[XYindex + 1] =  XTout;
    ///Calculate Y sweep data.
    int16_t Yout = (sinCalc * radarTrace) + 512;
    uint16_t YTout = ((Yout << 2) & 0x0FFF) | 0xA000; ///Update B and latch both A and B outputs.
    SPI_OUT[XYindex + 2] = (YTout >> 8);
    SPI_OUT[XYindex + 3] =  YTout;
    XYindex += 4;

    ///Calculate Intensity data.
    uint16_t Bright;
    if ( radarTrace>2 && radarTrace<512 ) {
        ///Draw Scale.
        if ((radarTrace&~0x003F)==radarTrace){
            Bright = scaleInten;
        } else {
            Bright = dimInten; ///Give some brightness.
        }

        float XaComp = Xout;
        float YaComp = Yout;
        float XdComp = (XaComp - 512) * (distFactRecip);
        float YdComp = (YaComp - 512) * (distFactRecip);
        for ( int b=0; b<MaxAircraft; ++b ) {
            if (O_ADSB_Database[b].AircraftAsleepTimer) {
                ///Get distance from point of measurement to aircrafts.
                float dist_sq =
                    sq( XdComp + O_ADSB_Database[b].CALC_Xdistance ) +
                    sq( YdComp - O_ADSB_Database[b].CALC_Ydistance )
                ;
                if ( dist_sq < blipy_sq ) {
                    Bright = blipInten; ///We got a ping! Full brightness!
                    break;
                }
            }
        }
    } else {
        Bright = blankInten; ///All the way dark.
    }
    uint16_t Intensity = ((Bright << 2) & 0x0FFF) | 0xF000; ///Update A&B with same data and latch.
    INTEN_OUT[INTENindex    ] = (Intensity >> 8);
    INTEN_OUT[INTENindex + 1] =  Intensity;
    INTENindex += 2;
    ++dspchCnt;
}

/** DAC Scope Loop **/
int F_DACscope() {
  if(!runDACscope) return 0;
  std::cout << "Starting DAC Scope Thread. \n";
  /** Open and configure SPI interface **/
  uint8_t OutMode = SPI_MODE_0;
  uint8_t OutBits = 8;
  uint8_t Default = 0;
  uint8_t TwoBytes = 2;
  uint32_t OutSpeed = 1000000;
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
  bool XYsel = 0;
  int iXY = 0;
  struct spi_ioc_transfer XYmesg[msgCnt];
  struct spi_ioc_transfer INTENmesg[intCnt];
  memset( & XYmesg, 0, sizeof(XYmesg));
  memset( & INTENmesg, 0, sizeof(INTENmesg));
  std::cout << "Generating initial SPI output tables. \n";
  ///Do XY
  for (int i = 0; i < msgCnt; i++) {
    XYmesg[i].rx_buf = (unsigned long) nullptr;
    XYmesg[i].bits_per_word = 8; // 0;
    XYmesg[i].tx_nbits = 0; // 0;
    XYmesg[i].word_delay_usecs = 0; // 0;
    XYmesg[i].pad = 0; // 0;
    if (i & 0x0001) {
      XYmesg[i].speed_hz = 32000000;
      XYmesg[i].tx_buf = (unsigned long) nullptr;
      XYmesg[i].len = 0;
      XYmesg[i].delay_usecs = 0; // 0
      XYmesg[i].cs_change = 1; // 0;
    } else {
      XYmesg[i].speed_hz = 10000000;
      XYmesg[i].len = 2;
      XYmesg[i].delay_usecs = 0; // 0
      XYmesg[i].cs_change = 0; // 0;
      if (XYsel) {
        XYmesg[i].tx_buf = (unsigned long) & SPI_OUT[iXY + 2];
        XYsel = 0;
      } else {
        XYmesg[i].tx_buf = (unsigned long) & SPI_OUT[iXY];
        XYsel = 1;
      }
      iXY += 4;
    }
  }
  ///Do INTENSITY
  iXY = 0;
  for (int i = 0; i < intCnt; i++) {
    INTENmesg[i].rx_buf = (unsigned long) nullptr;
    INTENmesg[i].bits_per_word = 8; // 0;
    INTENmesg[i].tx_nbits = 0; // 0;
    INTENmesg[i].word_delay_usecs = 0; // 0;
    INTENmesg[i].pad = 0; // 0;
    if (i & 0x0001) {
      INTENmesg[i].speed_hz = 32000000;
      INTENmesg[i].tx_buf = (unsigned long) nullptr;
      INTENmesg[i].len = 0;
      INTENmesg[i].delay_usecs = 2; // 0
      INTENmesg[i].cs_change = 1; // 0;
    } else {
      INTENmesg[i].speed_hz = 10000000;
      INTENmesg[i].len = 2;
      INTENmesg[i].delay_usecs = 0; // 0
      INTENmesg[i].cs_change = 0; // 0;
      INTENmesg[i].tx_buf = (unsigned long) & INTEN_OUT[iXY];
      iXY += 2;
    }
  }
  /******************************************/
  while (runDACscope) {
    ///Sweep
    for (radarSweep = 0; radarSweep < 360; radarSweep++) {
      ///Trace
      INTENindex = 0;
      XYindex = 0;
      float rad = DegToRad(radarSweep);
      float cosCalc = std::cos(rad);
      float sinCalc = std::sin(rad);
      for (radarTrace = -512; radarTrace < 512; radarTrace++) {
        ///Run a thread to calculate signals.
        ScopeCalc(cosCalc,sinCalc);
        /// Dispatch some data to the DACs
        if (dspchCnt == msgCnt) {
          if (ioctl(XY_SPI, SPI_IOC_MESSAGE(msgCnt), & XYmesg) < 0) {
            std::cout << "Sending data to /dev/spidev0.0 failed. errno: " << errno << " \n";
            runDACscope = 0; //Kill the display if write failure.
            break;
          }
          if (ioctl(INTEN_SPI, SPI_IOC_MESSAGE(intCnt), & INTENmesg) < 0) {
            std::cout << "Sending data to /dev/spidev0.1 failed. errno: " << errno << " \n";
            runDACscope = 0; //Kill the display if write failure.
            break;
          }
          INTENindex = 0;
          XYindex = 0;
          dspchCnt = 0;
        }
      }
      if (!runDACscope) break;
    }
  }
  close(XY_SPI);
  std::cout << "DAC Scope thread terminated. \n";
  return 0;
}
