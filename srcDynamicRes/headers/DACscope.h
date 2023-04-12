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

#ifndef DACscope_H
#define DACscope_H


#define DACscopeThreadCnt 2

///Screen parameters
extern int DEGsteps;
extern int DACresolution;
extern int XYresDivider;
///Actual DAC resolution
extern int XYresLowLim;
extern int XYresHighLim;
///Number of bulk messages to send.
extern int msgCnt;
///Number of raw DAC updates per screen sweep.
extern int XYpreCalcSweep;
///Bulk SPI system calls.
extern int bulkTransFactor;
///Allocation for how many 8 bit bytes to use for XY output buffer.
extern int SPI_OUT_Cnt;
///Intensity data
#define SyncCnt 1
///Allocation for how many 8 bit bytes to use for Intensity output buffer.
extern int SPI_INT_Cnt;

void PreCalcSweep(void);
int OutBufferSend(void);
extern int F_DACscope(void);
extern bool runDACscope;
extern float distFactor;
extern float blipScale;
extern int blankInten;
extern int dimInten;
extern int scaleInten;
extern int blipInten;
extern int SPIfreq;
extern int SPIdly;

class C_bulkSweepCalc{
public:
    bool dataReady=0;
    static int ScopeCalc(void);
};extern C_bulkSweepCalc* O_bulkSweepCalc;

#endif // PICTOVECTWAVE_H
