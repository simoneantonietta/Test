EESchema Schematic File Version 2
LIBS:interface-rescue
LIBS:saet
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:maxim
LIBS:interface-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 2
Title "RFIDReader - CS 3905"
Date "2016-05-17"
Rev "1.1"
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L C C10
U 1 1 5728C087
P 4200 2850
F 0 "C10" H 4200 2950 40  0000 L CNN
F 1 "470n" H 4206 2765 40  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 4238 2700 30  0001 C CNN
F 3 "" H 4200 2850 60  0000 C CNN
	1    4200 2850
	1    0    0    -1  
$EndComp
$Comp
L C C13
U 1 1 5728C088
P 5050 5350
F 0 "C13" H 5050 5450 40  0000 L CNN
F 1 "100n" H 5056 5265 40  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 5088 5200 30  0001 C CNN
F 3 "" H 5050 5350 60  0000 C CNN
	1    5050 5350
	1    0    0    -1  
$EndComp
$Comp
L C C12
U 1 1 5728C089
P 4750 4800
F 0 "C12" H 4750 4900 40  0000 L CNN
F 1 "470p" H 4756 4715 40  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 4788 4650 30  0001 C CNN
F 3 "" H 4750 4800 60  0000 C CNN
	1    4750 4800
	1    0    0    -1  
$EndComp
$Comp
L C C11
U 1 1 5728C08A
P 4450 4950
F 0 "C11" H 4450 5050 40  0000 L CNN
F 1 "10p" H 4456 4865 40  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 4488 4800 30  0001 C CNN
F 3 "" H 4450 4950 60  0000 C CNN
	1    4450 4950
	1    0    0    -1  
$EndComp
$Comp
L PMEG4005AEA D4
U 1 1 5728C08F
P 3200 2850
F 0 "D4" H 3200 2950 40  0000 C CNN
F 1 "PMEG4005AEA" H 3200 2750 40  0000 C CNN
F 2 "saet:SOD-323" H 2925 3150 60  0001 C CNN
F 3 "" H 3200 2850 60  0000 C CNN
F 4 "PMEG4005AEA" H 3300 3050 60  0001 C CNN "Product"
F 5 "DSG4005" H 3400 3150 60  0001 C CNN "Saet_code"
	1    3200 2850
	0    -1   -1   0   
$EndComp
$Comp
L INDUCTOR_3.3uH L1
U 1 1 5728C090
P 3750 3100
F 0 "L1" H 3750 3300 50  0000 C CNN
F 1 "INDUCTOR_3.3uH" H 3750 3200 50  0000 C CNN
F 2 "saet:IND_7.5x7.5" H 3600 3400 60  0001 C CNN
F 3 "" H 3750 3200 60  0000 C CNN
F 4 " 744 778 003" H 3850 3300 60  0001 C CNN "Product"
F 5 "TB3020" H 3950 3400 60  0001 C CNN "Saet_code"
	1    3750 3100
	1    0    0    -1  
$EndComp
$Comp
L B360A D5
U 1 1 5728C091
P 4200 3350
F 0 "D5" H 4200 3450 40  0000 C CNN
F 1 "B360A" H 4200 3250 40  0000 C CNN
F 2 "saet:SMA_Standard" H 4050 3650 60  0001 C CNN
F 3 "" H 4200 3350 60  0000 C CNN
F 4 "B360A-13-F" H 4300 3550 60  0001 C CNN "Product"
F 5 "DSB360A" H 4400 3650 60  0001 C CNN "Saet_code"
	1    4200 3350
	0    -1   -1   0   
$EndComp
$Comp
L R-EU R16
U 1 1 5728C094
P 4000 4150
F 0 "R16" H 3975 4250 60  0000 C CNN
F 1 "40.2k" H 4000 4050 60  0000 C CNN
F 2 "Resistors_SMD:R_0603" H 4075 4150 60  0001 C CNN
F 3 "" H 4075 4150 60  0000 C CNN
	1    4000 4150
	0    1    1    0   
$EndComp
$Comp
L R-EU R17
U 1 1 5728C095
P 4000 5150
F 0 "R17" H 3975 5250 60  0000 C CNN
F 1 "7.68k" H 4000 5050 60  0000 C CNN
F 2 "Resistors_SMD:R_0603" H 4075 5150 60  0001 C CNN
F 3 "" H 4075 5150 60  0000 C CNN
	1    4000 5150
	0    1    1    0   
$EndComp
$Comp
L R-EU R18
U 1 1 5728C096
P 4750 5300
F 0 "R18" H 4725 5400 60  0000 C CNN
F 1 "40.2k" H 4750 5200 60  0000 C CNN
F 2 "Resistors_SMD:R_0603" H 4825 5300 60  0001 C CNN
F 3 "" H 4825 5300 60  0000 C CNN
	1    4750 5300
	0    1    1    0   
$EndComp
$Comp
L R-EU R19
U 1 1 5728C098
P 6700 2250
F 0 "R19" H 6500 2150 60  0000 C CNN
F 1 "30.1k" H 6750 2150 60  0000 C CNN
F 2 "Resistors_SMD:R_0603" H 6775 2250 60  0001 C CNN
F 3 "" H 6775 2250 60  0000 C CNN
	1    6700 2250
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR033
U 1 1 5728C09E
P 5750 5750
F 0 "#PWR033" H 5750 5500 60  0001 C CNN
F 1 "GND" H 5750 5600 60  0000 C CNN
F 2 "" H 5750 5750 60  0000 C CNN
F 3 "" H 5750 5750 60  0000 C CNN
	1    5750 5750
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR034
U 1 1 5728C0A0
P 4200 3600
F 0 "#PWR034" H 4200 3350 60  0001 C CNN
F 1 "GND" H 4350 3500 60  0000 C CNN
F 2 "" H 4200 3600 60  0000 C CNN
F 3 "" H 4200 3600 60  0000 C CNN
	1    4200 3600
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR035
U 1 1 5728C0A1
P 6950 2350
F 0 "#PWR035" H 6950 2100 60  0001 C CNN
F 1 "GND" H 7100 2250 60  0000 C CNN
F 2 "" H 6950 2350 60  0000 C CNN
F 3 "" H 6950 2350 60  0000 C CNN
	1    6950 2350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR036
U 1 1 5728C0A2
P 2650 2700
F 0 "#PWR036" H 2650 2450 60  0001 C CNN
F 1 "GND" H 2650 2550 60  0000 C CNN
F 2 "" H 2650 2700 60  0000 C CNN
F 3 "" H 2650 2700 60  0000 C CNN
	1    2650 2700
	1    0    0    -1  
$EndComp
NoConn ~ 5050 2950
$Comp
L LT3501 U3
U 1 1 5728C0A3
P 5750 3550
F 0 "U3" H 5650 3500 60  0000 C CNN
F 1 "LT3501" H 5750 3400 60  0000 C CNN
F 2 "saet:SSOP-20_TP_4.4x6.5mm_Pitch0.65mm" H 9400 2800 60  0001 C CNN
F 3 "" H 9400 2800 60  0000 C CNN
F 4 "LT3501IFE#TRPBF" H 5750 3600 60  0001 C CNN "Product"
F 5 "IL1186" H 5850 3700 60  0001 C CNN "Saet_code"
	1    5750 3550
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR037
U 1 1 5728C0A4
P 3050 3750
F 0 "#PWR037" H 3050 3600 50  0001 C CNN
F 1 "+5V" H 3050 3890 50  0000 C CNN
F 2 "" H 3050 3750 50  0000 C CNN
F 3 "" H 3050 3750 50  0000 C CNN
	1    3050 3750
	1    0    0    -1  
$EndComp
NoConn ~ 6450 4000
NoConn ~ 5050 4000
$Comp
L CP C9
U 1 1 5728C0AC
P 3650 4800
F 0 "C9" H 3700 4900 40  0000 L CNN
F 1 "22u/20V" H 3700 4700 40  0000 L CNN
F 2 "Capacitors_Tantalum_SMD:TantalC_SizeC_EIA-6032_Wave" H 3750 4650 30  0001 C CNN
F 3 "" H 3650 4800 300 0000 C CNN
F 4 "T491C226K020AT" H 3650 4800 60  0001 C CNN "Product"
F 5 "CV0226" H 3650 4800 60  0001 C CNN "Saet_code"
	1    3650 4800
	1    0    0    -1  
$EndComp
$Comp
L CP C8
U 1 1 5728C0AE
P 2650 2400
F 0 "C8" H 2700 2500 40  0000 L CNN
F 1 "22u/20V" H 2700 2300 40  0000 L CNN
F 2 "Capacitors_Tantalum_SMD:TantalC_SizeC_EIA-6032_Wave" H 2750 2250 30  0001 C CNN
F 3 "" H 2650 2400 300 0000 C CNN
F 4 "T491C226K020AT" H 2650 2400 60  0001 C CNN "Product"
F 5 "CV0226" H 2650 2400 60  0001 C CNN "Saet_code"
	1    2650 2400
	1    0    0    -1  
$EndComp
Text HLabel 2050 2100 0    60   Input ~ 0
VIN
Wire Wire Line
	3200 2550 4750 2550
Wire Wire Line
	3200 2550 3200 2650
Wire Wire Line
	4200 2550 4200 2700
Connection ~ 4200 2550
Wire Wire Line
	4200 3000 4200 3150
Connection ~ 4200 3100
Wire Wire Line
	3500 3750 5050 3750
Wire Wire Line
	3500 3750 3500 3100
Wire Wire Line
	5750 4650 5750 5750
Wire Wire Line
	4450 4200 5050 4200
Wire Wire Line
	4750 4200 4750 4650
Wire Wire Line
	4450 5100 4450 5650
Connection ~ 5750 5650
Wire Wire Line
	4750 5550 4750 5650
Connection ~ 4750 5650
Wire Wire Line
	4450 4200 4450 4800
Connection ~ 4750 4200
Wire Wire Line
	3050 3850 5050 3850
Wire Wire Line
	4000 3100 4650 3100
Wire Wire Line
	4650 3100 4650 3300
Wire Wire Line
	4650 3300 5050 3300
Wire Wire Line
	4750 2550 4750 3050
Wire Wire Line
	4750 3050 5050 3050
Wire Wire Line
	4000 3900 4000 3850
Connection ~ 4000 3850
Wire Wire Line
	4000 4400 4000 4900
Wire Wire Line
	3650 3850 3650 4650
Connection ~ 3650 3850
Wire Wire Line
	4000 5650 4000 5400
Connection ~ 4450 5650
Wire Wire Line
	3650 4950 3650 5650
Connection ~ 4000 5650
Wire Wire Line
	4250 4100 4250 4600
Wire Wire Line
	4250 4100 5050 4100
Connection ~ 3200 3850
Wire Wire Line
	2650 2550 2650 2700
Wire Wire Line
	2050 2100 5850 2100
Wire Wire Line
	5850 2100 5850 2500
Wire Wire Line
	5550 2500 5550 2100
Connection ~ 5550 2100
Connection ~ 2650 2100
Wire Wire Line
	6450 2950 6450 2250
Wire Wire Line
	6950 2250 6950 2350
Wire Wire Line
	4200 3600 4200 3550
Wire Wire Line
	3050 3850 3050 3750
Wire Wire Line
	2650 2250 2650 2100
Wire Wire Line
	6450 4200 7050 4200
Wire Wire Line
	7050 4200 7050 4550
Wire Wire Line
	7850 5650 7850 4950
$Comp
L CP C18
U 1 1 5728C0AD
P 7850 4800
F 0 "C18" H 7900 4900 40  0000 L CNN
F 1 "22u/20V" H 7900 4700 40  0000 L CNN
F 2 "Capacitors_Tantalum_SMD:TantalC_SizeC_EIA-6032_Wave" H 7950 4650 30  0001 C CNN
F 3 "" H 7850 4800 300 0000 C CNN
F 4 "T491C226K020AT" H 7850 4800 60  0001 C CNN "Product"
F 5 "CV0226" H 7850 4800 60  0001 C CNN "Saet_code"
	1    7850 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	7250 4100 6450 4100
Wire Wire Line
	7250 4100 7250 4600
Wire Wire Line
	7850 3850 7850 4650
Connection ~ 7850 3850
Connection ~ 8300 3850
Wire Wire Line
	6450 3850 8650 3850
Wire Wire Line
	6750 3050 6450 3050
Wire Wire Line
	6750 2550 6750 3050
Wire Wire Line
	8300 2550 8300 2650
Wire Wire Line
	6750 2550 8300 2550
$Comp
L PMEG4005AEA D7
U 1 1 5728C09A
P 8300 2850
F 0 "D7" H 8300 2950 40  0000 C CNN
F 1 "PMEG4005AEA" H 8300 2750 40  0000 C CNN
F 2 "saet:SOD-323" H 8025 3150 60  0001 C CNN
F 3 "" H 8300 2850 60  0000 C CNN
F 4 "PMEG4005AEA" H 8400 3050 60  0001 C CNN "Product"
F 5 "DSG4005" H 8500 3150 60  0001 C CNN "Saet_code"
	1    8300 2850
	0    -1   -1   0   
$EndComp
Wire Wire Line
	8000 3750 6450 3750
Wire Wire Line
	8000 3750 8000 3100
Wire Wire Line
	7300 3600 7300 3550
$Comp
L GND #PWR038
U 1 1 5728C09F
P 7300 3600
F 0 "#PWR038" H 7300 3350 60  0001 C CNN
F 1 "GND" H 7450 3500 60  0000 C CNN
F 2 "" H 7300 3600 60  0000 C CNN
F 3 "" H 7300 3600 60  0000 C CNN
	1    7300 3600
	1    0    0    -1  
$EndComp
Connection ~ 7300 2550
Wire Wire Line
	7300 2550 7300 2700
Wire Wire Line
	6850 3300 6450 3300
Wire Wire Line
	6850 3100 6850 3300
Wire Wire Line
	6850 3100 7500 3100
Wire Wire Line
	7300 3000 7300 3150
Connection ~ 7300 3100
$Comp
L B360A D6
U 1 1 5728C097
P 7300 3350
F 0 "D6" H 7300 3450 40  0000 C CNN
F 1 "B360A" H 7300 3250 40  0000 C CNN
F 2 "saet:SMA_Standard" H 7150 3650 60  0001 C CNN
F 3 "" H 7300 3350 60  0000 C CNN
F 4 "B360A-13-F" H 7400 3550 60  0001 C CNN "Product"
F 5 "DSB360A" H 7500 3650 60  0001 C CNN "Saet_code"
	1    7300 3350
	0    -1   -1   0   
$EndComp
$Comp
L INDUCTOR_3.3uH L2
U 1 1 5728C099
P 7750 3100
F 0 "L2" H 7750 3300 50  0000 C CNN
F 1 "INDUCTOR_3.3uH" H 7750 3200 50  0000 C CNN
F 2 "saet:IND_7.5x7.5" H 7600 3400 60  0001 C CNN
F 3 "" H 7750 3200 60  0000 C CNN
F 4 " 744 778 003" H 7850 3300 60  0001 C CNN "Product"
F 5 "TB3020" H 7950 3400 60  0001 C CNN "Saet_code"
	1    7750 3100
	1    0    0    -1  
$EndComp
$Comp
L C C17
U 1 1 5728C08B
P 7300 2850
F 0 "C17" H 7300 2950 40  0000 L CNN
F 1 "0.47u" H 7306 2765 40  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 7338 2700 30  0001 C CNN
F 3 "" H 7300 2850 60  0000 C CNN
	1    7300 2850
	-1   0    0    -1  
$EndComp
$Comp
L CONN_01X02 P4
U 1 1 57290D03
P 8950 4300
F 0 "P4" H 9028 4394 50  0000 L CNN
F 1 "CONN_01X02" H 9028 4303 50  0000 L CNN
F 2 "saet:Adimpex_3.5_2_ways" H 8950 4300 50  0001 C CNN
F 3 "" H 8950 4300 50  0000 C CNN
F 4 "X" H 9028 4204 60  0000 L CNN "NOT_MOUNT"
	1    8950 4300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR039
U 1 1 57290DE2
P 8750 4450
F 0 "#PWR039" H 8750 4200 60  0001 C CNN
F 1 "GND" H 8750 4300 60  0000 C CNN
F 2 "" H 8750 4450 60  0000 C CNN
F 3 "" H 8750 4450 60  0000 C CNN
	1    8750 4450
	1    0    0    -1  
$EndComp
Wire Wire Line
	8750 4350 8750 4450
Wire Wire Line
	8300 4250 8750 4250
Text Label 4350 3100 0    60   ~ 0
_pwrA1
Text Label 4550 3750 0    60   ~ 0
_pwrA2
Text Label 6500 3300 0    60   ~ 0
_pwrA3
Text Label 6600 3750 0    60   ~ 0
_pwrA4
Text Label 3650 2550 0    60   ~ 0
_pwrA5
Text Label 7650 2550 0    60   ~ 0
_pwrA6
Text Label 7950 3850 0    60   ~ 0
_pwrB1
$Comp
L +5V #PWR040
U 1 1 5BC70DED
P 8650 3750
F 0 "#PWR040" H 8650 3600 50  0001 C CNN
F 1 "+5V" H 8650 3890 50  0000 C CNN
F 2 "" H 8650 3750 50  0000 C CNN
F 3 "" H 8650 3750 50  0000 C CNN
	1    8650 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	8650 3850 8650 3750
Wire Wire Line
	6450 5100 5050 5100
Wire Wire Line
	6450 4300 6450 5100
Connection ~ 5050 5100
Connection ~ 5050 5650
Wire Wire Line
	5050 4300 5050 5200
Wire Wire Line
	3200 3050 3200 3850
Wire Wire Line
	8300 3050 8300 4250
Wire Wire Line
	5050 5650 5050 5500
Wire Wire Line
	3650 5650 7850 5650
Wire Wire Line
	4750 4950 4750 5050
Wire Wire Line
	7050 4550 4750 4550
Connection ~ 4750 4550
Wire Wire Line
	7250 4600 4000 4600
Connection ~ 4000 4600
Connection ~ 4250 4600
$EndSCHEMATC