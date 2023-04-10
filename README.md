# ADSB_Sweeper

Copyright: Jarrett Cigainero 2023

This program is provided without any warranty.

A program to generate the proper analog signals to send to a real RADAR scope tube using ADS-B data piped from dump1090.

** Not for use in air navigation, in-flight situational awareness, or in-flight traffic monitoring. **

Uses the SPI output to send data to two LTC1662 DACs to generate real-time sweep and intensity data for showing ADS-B traffic on an XY scope, or oscilloscope in XY mode. The XY DAC is conntected to CE0, and the intensity DAC is connected to CE1 with both of it's outputs copied so it doesn't matter which one you connect.

I've updated the program to alternate between DAC chips on CE0. There is more control of intensity data this way, but requires an external circuit to
switch between the two chips using a flip-flop. I'm using a 74s74 dual flip-flop and a 74ls32 quad OR gate for this. I've included a schematic.

The current version of this software is slow, and pushes Linux and the Raspi to it's limits in terms of speed. I plan on further optimizations later on, but as it is right now the sweep speed is what you get.

This program requires dump1090 or equivilant data to be piped into it to function. You must also enable the SPI interface on the Raspi. I'm using a Raspi 3b. I have no idea what kind of performance you will get on other devices.

If you don't see /dev/spidev0.0 and /dev/spidev0.1 then it isn't enabled or something else is wrong.

Usage Example:

dump1090-mutability | ./adsbSweeper -D -C -a 32.757541 -o -97.076364 -R 5 -m 5

           **This program is not intended for in-flight use. **
           
    **Do not use for air navigation or in-flight traffic monitoring. **

-h :: This Help Text.

-D :: Enable DAC based RADAR scope output.

-C :: Enable CLI based RADAR scope output.

-m :: Max range of aircraft in nm before being deleted from database. Default: 4nm

-R :: Scope Radius. Trace Length in nm. Default: 4nm

-B :: Blip-Size Scale-Factor. Default: 1

-a :: Manual Latitude setting.

-o :: Manual Longitude setting.

-S :: Aircraft 'Sleep' Time in half seconds. How long an aircraft stays in database before being deleted. Default: <20> 10 seconds.
      
-M :: Max number of Aircraft before new ones are rejected. Default: 26

-b :: Blip Brightness Value. Default = 0

-d :: Trace Brightness Value. Default = 512

-s :: Scale Brightness Value. Default = 470

-c :: Blanking Value. Default = 1023

