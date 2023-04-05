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


///Default max msgCnt size seems to be 128
#define msgCnt 8        ///8 seems to work with 1024 resolution. Nothing else works and I'm not sure why.
#define intCnt 4
#define SPI_OUT_Cnt 32
#define SPI_INT_Cnt 16
extern int F_DACscope(void);
extern bool runDACscope;
extern float distFactor;
extern float blipScale;
extern int blankInten;
extern int dimInten;
extern int scaleInten;
extern int blipInten;

#endif // PICTOVECTWAVE_H
