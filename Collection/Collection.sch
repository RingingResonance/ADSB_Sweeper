EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L 74xx:74LS74 U?
U 1 1 64338365
P 4900 3850
F 0 "U?" H 5100 4150 50  0000 C CNN
F 1 "74LS74" H 4650 4150 50  0000 C CNN
F 2 "" H 4900 3850 50  0001 C CNN
F 3 "74xx/74hc_hct74.pdf" H 4900 3850 50  0001 C CNN
	1    4900 3850
	1    0    0    -1  
$EndComp
$Comp
L 74xx:74LS32 U?
U 1 1 64338AA6
P 5800 3650
F 0 "U?" H 5800 4000 50  0000 C CNN
F 1 "74LS32" H 5800 3900 50  0000 C CNN
F 2 "" H 5800 3650 50  0001 C CNN
F 3 "http://www.ti.com/lit/gpn/sn74LS32" H 5800 3650 50  0001 C CNN
	1    5800 3650
	1    0    0    -1  
$EndComp
$Comp
L 74xx:74LS32 U?
U 2 1 6433A939
P 5800 4050
F 0 "U?" H 5800 4400 50  0000 C CNN
F 1 "74LS32" H 5800 4300 50  0000 C CNN
F 2 "" H 5800 4050 50  0001 C CNN
F 3 "http://www.ti.com/lit/gpn/sn74LS32" H 5800 4050 50  0001 C CNN
	2    5800 4050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5500 4150 5400 4150
Wire Wire Line
	5400 4150 5400 3550
Wire Wire Line
	5400 3550 5500 3550
Wire Wire Line
	5200 3950 5500 3950
Wire Wire Line
	5200 3750 5500 3750
Text Label 6100 3650 0    50   ~ 0
XYdax
Text Label 6100 4050 0    50   ~ 0
IntensityDAC
Wire Wire Line
	4600 3750 4300 3750
Wire Wire Line
	4300 3750 4300 4200
Wire Wire Line
	4300 4200 5200 4200
Wire Wire Line
	5200 4200 5200 3950
Connection ~ 5200 3950
Text Label 4150 3850 2    50   ~ 0
CE0
Text Label 4600 4150 2    50   ~ 0
CE1
$Comp
L power:+5V #PWR?
U 1 1 64340E0D
P 4900 3550
F 0 "#PWR?" H 4900 3400 50  0001 C CNN
F 1 "+5V" H 4950 3750 50  0000 C CNN
F 2 "" H 4900 3550 50  0001 C CNN
F 3 "" H 4900 3550 50  0001 C CNN
	1    4900 3550
	1    0    0    -1  
$EndComp
Wire Wire Line
	4600 4150 4900 4150
Wire Wire Line
	4150 3850 4200 3850
Wire Wire Line
	5400 3550 5400 3200
Wire Wire Line
	5400 3200 4200 3200
Wire Wire Line
	4200 3200 4200 3850
Connection ~ 5400 3550
Connection ~ 4200 3850
Wire Wire Line
	4200 3850 4600 3850
Wire Notes Line
	6600 3000 6600 4350
Wire Notes Line
	6600 4350 3950 4350
Wire Notes Line
	3950 4350 3950 3000
Wire Notes Line
	3950 3000 6600 3000
$EndSCHEMATC