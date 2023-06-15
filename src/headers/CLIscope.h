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

#ifndef CLIscope_H
#define CLIscope_H

#define zoomFactorX 10
#define zoomFactorY 5

static char DSP_ID[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ%"};
extern int CLIrScope(void);
extern bool CfullScope;
extern int refTime;

#endif // PICTOVECTWAVE_H
