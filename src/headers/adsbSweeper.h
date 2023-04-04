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

#ifndef asdbSweeper_H
#define asdbSweeper_H

int GetArgs(int, char**);

const char *helpText[]{
"\nThis program is provided without any warranty.\n"
"Copyright: Jarrett Cigainero October 2023 \n"
"\n     **This program is not intended for in-flight use. **\n"
"\n **Do not use for navigation or in-flight traffic monitoring. **\n"
"\n"
"-h :: This Help Text.\n"
"-D :: Enable DAC based RADAR scope output.\n"
"-C :: Enable CLI based RADAR scope output.\n"
"-m :: Max range of aircraft in nm before being deleted from database. Default: 4nm\n"
"-R :: Scope Radius. Trace Length in nm. Default: 4nm\n"
"-B :: Blip-Size Scale-Factor. Default: 1"
"-a :: Manual Latitude setting.\n"
"-o :: Manual Longitude setting.\n"
"-S :: Aircraft 'Sleep' Time in half seconds. How long an aircraft stays\n"
"      in database before being deleted. Default: <20> 10 seconds. \n"
"-M :: Max number of Aircraft before new ones are rejected. Default: 26\n"
"\n"
};

#endif // PICTOVECTWAVE_H
