EESchema Schematic File Version 2
LIBS:3852_6-rescue
LIBS:Contatto868-3V-rescue
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
LIBS:3852_6-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 2
Title "Magnetic Contact 3V on 868.3 MHz - CS 3862"
Date "mercoledì 03 giugno 2015"
Rev "2.0"
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L GND-RESCUE-Contatto868-3V #PWR01
U 1 1 53F1C79C
P 2150 1400
F 0 "#PWR01" H 2150 1400 30  0001 C CNN
F 1 "GND" H 2150 1330 30  0001 C CNN
F 2 "" H 2150 1400 60  0000 C CNN
F 3 "" H 2150 1400 60  0000 C CNN
	1    2150 1400
	1    0    0    -1  
$EndComp
$Comp
L CP1-RESCUE-Contatto868-3V C16
U 1 1 53F1E72C
P 2800 1500
F 0 "C16" H 2850 1600 59  0000 L CNN
F 1 "47u/50V" H 2800 1350 59  0000 L CNN
F 2 "Capacitors_SMD:C_0805" H 2800 1500 60  0001 C CNN
F 3 "~" H 2800 1500 60  0000 C CNN
F 4 "CS2476" H 2800 1500 60  0001 C CNN "Saet_code"
	1    2800 1500
	-1   0    0    -1  
$EndComp
$Comp
L VCC #PWR02
U 1 1 53F30917
P 6500 2700
F 0 "#PWR02" H 6500 2800 30  0001 C CNN
F 1 "VCC" H 6500 2800 30  0000 C CNN
F 2 "" H 6500 2700 60  0000 C CNN
F 3 "" H 6500 2700 60  0000 C CNN
	1    6500 2700
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR03
U 1 1 53F309D7
P 5600 3300
F 0 "#PWR03" H 5600 3300 30  0001 C CNN
F 1 "GND" H 5600 3230 30  0001 C CNN
F 2 "" H 5600 3300 60  0000 C CNN
F 3 "" H 5600 3300 60  0000 C CNN
	1    5600 3300
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C26
U 1 1 53F30A1F
P 5600 3050
F 0 "C26" H 5600 3150 59  0000 L CNN
F 1 "100n" H 5606 2965 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 5638 2900 30  0001 C CNN
F 3 "~" H 5600 3050 60  0000 C CNN
F 4 "CSA104 EXT" H 5600 3050 60  0001 C CNN "Saet_code"
	1    5600 3050
	-1   0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR04
U 1 1 53F30BA0
P 6500 1050
F 0 "#PWR04" H 6500 1050 30  0001 C CNN
F 1 "GND" H 6500 980 30  0001 C CNN
F 2 "" H 6500 1050 60  0000 C CNN
F 3 "" H 6500 1050 60  0000 C CNN
	1    6500 1050
	-1   0    0    -1  
$EndComp
$Comp
L CRYSTAL QZ3
U 1 1 53F30EC1
P 7600 1500
F 0 "QZ3" H 7600 1650 59  0000 C CNN
F 1 "32KHz" H 7600 1350 59  0000 C CNN
F 2 "Crystals:Crystal_SMD_3215-2pin_3.2x1.5mm" H 7600 1500 60  0001 C CNN
F 3 "~" H 7600 1500 60  0000 C CNN
F 4 "YQ0323AS" H 7600 1500 60  0001 C CNN "Saet_code"
	1    7600 1500
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C29
U 1 1 53F31058
P 7150 1200
F 0 "C29" H 7150 1300 59  0000 L CNN
F 1 "56p" H 7156 1115 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 7188 1050 30  0001 C CNN
F 3 "~" H 7150 1200 60  0000 C CNN
F 4 "CSA560 EXT" H 7150 1200 60  0001 C CNN "Saet_code"
	1    7150 1200
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C28
U 1 1 53F310A9
P 8050 1200
F 0 "C28" H 8050 1300 59  0000 L CNN
F 1 "56p" H 8056 1115 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 8088 1050 30  0001 C CNN
F 3 "~" H 8050 1200 60  0000 C CNN
F 4 "CSA560 EXT" H 8050 1200 60  0001 C CNN "Saet_code"
	1    8050 1200
	1    0    0    -1  
$EndComp
$Comp
L LED DL1
U 1 1 53F31549
P 8300 2750
F 0 "DL1" H 8300 2850 59  0000 C CNN
F 1 "LEDR-LHN974" H 8350 2600 59  0000 C CNN
F 2 "LEDs:LED_0805" H 8300 2750 60  0001 C CNN
F 3 "~" H 8300 2750 60  0000 C CNN
F 4 "LE7011" H 8300 2750 60  0001 C CNN "Saet_code"
	1    8300 2750
	-1   0    0    -1  
$EndComp
$Comp
L R-RESCUE-Contatto868-3V R10
U 1 1 53F315E5
P 7750 2750
F 0 "R10" V 7850 2750 55  0000 C CNN
F 1 "470" V 7750 2750 55  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 7680 2750 30  0000 C CNN
F 3 "~" H 7750 2750 30  0000 C CNN
F 4 "RSA471 EXT" V 7750 2750 60  0001 C CNN "Saet_code"
	1    7750 2750
	0    -1   -1   0   
$EndComp
$Comp
L R-RESCUE-Contatto868-3V R9
U 1 1 53F31701
P 10250 3400
F 0 "R9" V 10350 3400 59  0000 C CNN
F 1 "47K" V 10250 3400 55  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 10180 3400 30  0000 C CNN
F 3 "~" H 10250 3400 30  0000 C CNN
F 4 "RSA473 EXT" V 10250 3400 60  0001 C CNN "Saet_code"
	1    10250 3400
	-1   0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C34
U 1 1 53F31710
P 10250 3950
F 0 "C34" H 10250 4050 59  0000 L CNN
F 1 "2n2" H 10250 3850 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 10288 3800 30  0001 C CNN
F 3 "~" H 10250 3950 60  0000 C CNN
F 4 "CSA222 EXT" H 10250 3950 60  0001 C CNN "Saet_code"
	1    10250 3950
	-1   0    0    -1  
$EndComp
$Comp
L VCC #PWR05
U 1 1 53F31735
P 10250 3100
F 0 "#PWR05" H 10250 3200 30  0001 C CNN
F 1 "VCC" H 10250 3200 30  0000 C CNN
F 2 "" H 10250 3100 60  0000 C CNN
F 3 "" H 10250 3100 60  0000 C CNN
	1    10250 3100
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR06
U 1 1 53F317E1
P 10250 4200
F 0 "#PWR06" H 10250 4200 30  0001 C CNN
F 1 "GND" H 10250 4130 30  0001 C CNN
F 2 "" H 10250 4200 60  0000 C CNN
F 3 "" H 10250 4200 60  0000 C CNN
	1    10250 4200
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X08 CN3
U 1 1 53F32192
P 10300 5250
F 0 "CN3" H 10300 4800 59  0000 C CNN
F 1 "STRIP Prog." V 10400 5250 59  0000 C CNN
F 2 "saet:strip_pad_x8" H 10300 5250 60  0001 C CNN
F 3 "" H 10300 5250 60  0000 C CNN
	1    10300 5250
	1    0    0    1   
$EndComp
$Comp
L VCC #PWR07
U 1 1 53F323D0
P 10050 4800
F 0 "#PWR07" H 10050 4900 30  0001 C CNN
F 1 "VCC" H 10050 4900 30  0000 C CNN
F 2 "" H 10050 4800 60  0000 C CNN
F 3 "" H 10050 4800 60  0000 C CNN
	1    10050 4800
	1    0    0    -1  
$EndComp
NoConn ~ 6550 5200
NoConn ~ 6850 5200
NoConn ~ 7050 5200
NoConn ~ 8300 4200
NoConn ~ 6200 3000
NoConn ~ 5700 4850
$Comp
L GND-RESCUE-Contatto868-3V #PWR08
U 1 1 53F386A7
P 7400 2850
F 0 "#PWR08" H 7400 2850 30  0001 C CNN
F 1 "GND" H 7400 2780 30  0001 C CNN
F 2 "" H 7400 2850 60  0000 C CNN
F 3 "" H 7400 2850 60  0000 C CNN
	1    7400 2850
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR09
U 1 1 53F3885F
P 10050 5700
F 0 "#PWR09" H 10050 5700 30  0001 C CNN
F 1 "GND" H 10050 5630 30  0001 C CNN
F 2 "" H 10050 5700 60  0000 C CNN
F 3 "" H 10050 5700 60  0000 C CNN
	1    10050 5700
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR010
U 1 1 53F38C81
P 4500 1950
F 0 "#PWR010" H 4500 1950 30  0001 C CNN
F 1 "GND" H 4500 1880 30  0001 C CNN
F 2 "" H 4500 1950 60  0000 C CNN
F 3 "" H 4500 1950 60  0000 C CNN
	1    4500 1950
	1    0    0    -1  
$EndComp
Text GLabel 9800 1700 2    60   Input ~ 0
CS
Text GLabel 9800 1900 2    60   Input ~ 0
CLK
Text GLabel 9800 2100 2    60   Input ~ 0
SI
Text GLabel 9800 2300 2    60   Input ~ 0
SO
Text GLabel 9800 2500 2    60   Input ~ 0
IRQ
Text GLabel 9800 2700 2    60   Input ~ 0
PD
$Sheet
S 9800 1600 1000 1200
U 53F4C1B5
F0 "Radio" 50
F1 "CONT-3V-radio.sch" 50
$EndSheet
$Comp
L MSP430G2433IRHN32 U2
U 1 1 53F5B58D
P 7000 4100
F 0 "U2" H 5900 5100 59  0000 C CNN
F 1 "MSP430G2433IRHN32" H 7700 5100 59  0000 C CNN
F 2 "Housings_DFN_QFN:QFN-32-1EP_5x5mm_Pitch0.5mm" H 6000 3100 59  0001 C CIN
F 3 "~" H 7000 4100 60  0000 C CNN
F 4 "IU1164" H 7000 4100 60  0001 C CNN "Saet_code"
	1    7000 4100
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR011
U 1 1 5460ECD8
P 4500 1100
F 0 "#PWR011" H 4500 1200 30  0001 C CNN
F 1 "VCC" H 4500 1200 30  0000 C CNN
F 2 "" H 4500 1100 60  0000 C CNN
F 3 "" H 4500 1100 60  0000 C CNN
	1    4500 1100
	1    0    0    -1  
$EndComp
Text Notes 9500 1700 0    47   ~ 0
U1(16)
Text Notes 9500 1900 0    47   ~ 0
U1(15)
Text Notes 9500 2100 0    47   ~ 0
U1(14)
Text Notes 9500 2300 0    47   ~ 0
U1(13)
Text Notes 9500 2500 0    47   ~ 0
U1(17)
Text Notes 9500 2700 0    47   ~ 0
U1(20)
$Comp
L R-RESCUE-Contatto868-3V R32
U 1 1 54621C28
P 6950 6400
F 0 "R32" V 7050 6400 59  0000 C CNN
F 1 "0" V 6957 6401 55  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 6880 6400 30  0001 C CNN
F 3 "" H 6950 6400 30  0000 C CNN
F 4 "RSA000 EXT" V 6950 6400 60  0001 C CNN "Saet_code"
	1    6950 6400
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR012
U 1 1 54621EBE
P 6950 6750
F 0 "#PWR012" H 6950 6750 30  0001 C CNN
F 1 "GND" H 6950 6680 30  0001 C CNN
F 2 "" H 6950 6750 60  0000 C CNN
F 3 "" H 6950 6750 60  0000 C CNN
	1    6950 6750
	1    0    0    -1  
$EndComp
$Comp
L CP1-RESCUE-Contatto868-3V C22
U 1 1 5462CB7B
P 4050 1500
F 0 "C22" H 4100 1600 59  0000 L CNN
F 1 "47u/50V" H 4050 1350 59  0000 L CNN
F 2 "Capacitors_SMD:C_0805" H 4050 1500 60  0001 C CNN
F 3 "~" H 4050 1500 60  0000 C CNN
F 4 "CS2476" H 4050 1500 60  0001 C CNN "Saet_code"
	1    4050 1500
	-1   0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C2
U 1 1 5464CB0E
P 3250 1500
F 0 "C2" H 3250 1600 59  0000 L CNN
F 1 "4u7" H 3250 1350 59  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 3288 1350 30  0001 C CNN
F 3 "~" H 3250 1500 60  0000 C CNN
F 4 "CS1475 EXT" H 3250 1500 60  0001 C CNN "Saet_code"
	1    3250 1500
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C4
U 1 1 5464CD24
P 4500 1500
F 0 "C4" H 4500 1600 59  0000 L CNN
F 1 "10u" H 4500 1350 59  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 4538 1350 30  0001 C CNN
F 3 "~" H 4500 1500 60  0000 C CNN
F 4 "CS1106 EXT" H 4500 1500 60  0001 C CNN "Saet_code"
	1    4500 1500
	1    0    0    -1  
$EndComp
NoConn ~ 7250 5200
NoConn ~ 7150 5200
NoConn ~ 5700 4750
NoConn ~ 6750 5200
$Comp
L CONN_01X04 P1
U 1 1 5BA21ECB
P 2350 3750
F 0 "P1" H 2350 4000 50  0000 C CNN
F 1 "SERIAL CONNECTOR" V 2450 3600 50  0000 C CNN
F 2 "Connectors_Molex:Connector_Molex_PicoBlade_53047-0410" H 2350 3750 50  0001 C CNN
F 3 "" H 2350 3750 50  0000 C CNN
	1    2350 3750
	-1   0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR013
U 1 1 5BA21F61
P 3000 3800
F 0 "#PWR013" H 3000 3800 30  0001 C CNN
F 1 "GND-RESCUE-Contatto868-3V" H 3000 3730 30  0001 C CNN
F 2 "" H 3000 3800 60  0000 C CNN
F 3 "" H 3000 3800 60  0000 C CNN
	1    3000 3800
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 5BA22338
P 4250 4150
F 0 "R3" V 4330 4150 50  0000 C CNN
F 1 "1k" V 4250 4150 50  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 4180 4150 50  0001 C CNN
F 3 "" H 4250 4150 50  0000 C CNN
F 4 "RSA102 EXT" V 4250 4150 60  0001 C CNN "Saet_code"
	1    4250 4150
	0    1    1    0   
$EndComp
$Comp
L R R4
U 1 1 5BA223AC
P 3400 3850
F 0 "R4" V 3480 3850 50  0000 C CNN
F 1 "2,2M" V 3400 3850 50  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 3330 3850 50  0001 C CNN
F 3 "" H 3400 3850 50  0000 C CNN
F 4 "RSA103 EXT" V 3400 3850 60  0001 C CNN "Saet_code"
	1    3400 3850
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 5BA22409
P 3750 3850
F 0 "R2" V 3830 3850 50  0000 C CNN
F 1 "1k" V 3750 3850 50  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 3680 3850 50  0001 C CNN
F 3 "" H 3750 3850 50  0000 C CNN
F 4 "RSA102 EXT" V 3750 3850 60  0001 C CNN "Saet_code"
	1    3750 3850
	-1   0    0    1   
$EndComp
Text Label 5450 4150 0    60   ~ 0
RX
NoConn ~ 5700 4350
$Comp
L Q_NMOS_DGS Q1
U 1 1 5BA24173
P 3750 4400
F 0 "Q1" H 4050 4450 50  0000 R CNN
F 1 "Q_NMOS_DGS" H 4400 4350 50  0000 R CNN
F 2 "TO_SOT_Packages_SMD:SC-70" H 3950 4500 50  0001 C CNN
F 3 "" H 3750 4400 50  0000 C CNN
F 4 "QT3134" H 3750 4400 60  0001 C CNN "Saet_code"
	1    3750 4400
	0    -1   1    0   
$EndComp
Text Label 5450 4250 0    60   ~ 0
TX
Wire Wire Line
	2050 1200 4500 1200
Connection ~ 4050 1200
Connection ~ 2800 1200
Wire Wire Line
	6500 2700 6500 3000
Wire Wire Line
	5600 2750 6500 2750
Wire Wire Line
	6500 950  6500 1050
Wire Wire Line
	6500 950  8050 950 
Wire Wire Line
	8050 1500 7900 1500
Wire Wire Line
	6800 1500 7300 1500
Wire Wire Line
	8050 1400 8050 1850
Wire Wire Line
	7150 1500 7150 1400
Wire Wire Line
	8050 950  8050 1000
Wire Wire Line
	8100 2750 8000 2750
Wire Wire Line
	10250 3100 10250 3150
Wire Wire Line
	10250 3650 10250 3750
Wire Wire Line
	10250 4150 10250 4200
Wire Wire Line
	10100 5100 9850 5100
Wire Wire Line
	7150 950  7150 1000
Connection ~ 7150 950 
Wire Wire Line
	6800 3000 6800 1500
Connection ~ 7150 1500
Wire Wire Line
	6900 3000 6900 1850
Wire Wire Line
	6900 1850 8050 1850
Connection ~ 8050 1500
Wire Wire Line
	6700 950  6700 3000
Wire Wire Line
	6600 3000 6600 2850
Wire Wire Line
	6600 2850 6700 2850
Connection ~ 6700 2850
Wire Wire Line
	8300 4100 8600 4100
Wire Wire Line
	8600 4100 8600 2750
Wire Wire Line
	9950 5000 10100 5000
Wire Wire Line
	10050 4800 10050 4900
Wire Wire Line
	10050 4900 10100 4900
Wire Wire Line
	8300 3700 10250 3700
Connection ~ 10250 3700
Wire Wire Line
	8300 3600 9950 3600
Wire Wire Line
	9950 3600 9950 5000
Wire Wire Line
	9850 5100 9850 3700
Connection ~ 9850 3700
Wire Wire Line
	8300 3800 9750 3800
Wire Wire Line
	9750 3800 9750 5500
Wire Wire Line
	9750 5500 10100 5500
Wire Wire Line
	8300 3900 9650 3900
Wire Wire Line
	9650 3900 9650 5400
Wire Wire Line
	9650 5400 10100 5400
Wire Wire Line
	9250 2700 9250 4000
Wire Wire Line
	9150 4300 8300 4300
Wire Wire Line
	9150 2500 9150 4300
Wire Wire Line
	10100 5200 9450 5200
Wire Wire Line
	10100 5300 9550 5300
Wire Wire Line
	9450 5200 9450 5700
Wire Wire Line
	9450 5700 5100 5700
Wire Wire Line
	9550 5300 9550 5600
Wire Wire Line
	9550 5600 5200 5600
Wire Wire Line
	9050 5400 5300 5400
Wire Wire Line
	9050 1700 9050 5400
Wire Wire Line
	5100 5700 5100 4450
Wire Wire Line
	5100 4450 5700 4450
Wire Wire Line
	5700 4550 5200 4550
Wire Wire Line
	5200 4550 5200 5600
Wire Wire Line
	5300 5400 5300 4650
Connection ~ 6500 2750
Connection ~ 6700 950 
Wire Wire Line
	5300 4650 5700 4650
Wire Wire Line
	9250 4000 8300 4000
Wire Wire Line
	5600 3250 5600 3300
Wire Wire Line
	10050 5600 10050 5700
Wire Wire Line
	2800 1800 4500 1800
Wire Wire Line
	2150 1300 2150 1400
Wire Wire Line
	9250 2700 9800 2700
Wire Wire Line
	9150 2500 9800 2500
Wire Wire Line
	9800 1700 9050 1700
Wire Wire Line
	6300 3000 6300 2050
Wire Wire Line
	10100 5600 10050 5600
Wire Wire Line
	4050 1800 4050 1700
Wire Wire Line
	2800 1200 2800 1300
Wire Wire Line
	2050 1300 2150 1300
Wire Wire Line
	6400 2850 6500 2850
Connection ~ 6500 2850
Wire Wire Line
	5600 2750 5600 2850
Wire Wire Line
	7400 2750 7500 2750
Wire Wire Line
	8600 2750 8500 2750
Wire Wire Line
	9150 2050 9150 1900
Wire Wire Line
	9150 1900 9800 1900
Wire Wire Line
	9250 2200 9250 2100
Wire Wire Line
	9250 2100 9800 2100
Wire Wire Line
	9350 2350 9350 2300
Wire Wire Line
	9350 2300 9800 2300
Wire Wire Line
	6950 5200 6950 6150
Wire Wire Line
	6950 6650 6950 6750
Wire Wire Line
	3250 1300 3250 1200
Connection ~ 3250 1200
Wire Wire Line
	3250 1700 3250 1800
Connection ~ 3250 1800
Wire Wire Line
	4500 1700 4500 1950
Connection ~ 4050 1800
Wire Wire Line
	4050 1200 4050 1300
Wire Wire Line
	4500 1100 4500 1300
Connection ~ 4500 1200
Connection ~ 4500 1800
Wire Wire Line
	2800 1700 2800 1800
Wire Wire Line
	7400 2750 7400 2850
Wire Wire Line
	6300 2050 9150 2050
Wire Wire Line
	8900 2350 8900 3900
Connection ~ 8900 3900
Wire Wire Line
	8800 3800 8800 2200
Connection ~ 8800 3800
Wire Wire Line
	8800 2200 9250 2200
Wire Wire Line
	8900 2350 9350 2350
Wire Wire Line
	3000 3700 3000 3800
Wire Wire Line
	2550 3700 3000 3700
Wire Wire Line
	2550 3800 2900 3800
Wire Wire Line
	2550 3900 2800 3900
Wire Wire Line
	4400 4150 5700 4150
Wire Wire Line
	5700 4250 4850 4250
Wire Wire Line
	4850 4500 3950 4500
Wire Wire Line
	2800 4500 3550 4500
Wire Wire Line
	4850 4250 4850 4500
Wire Wire Line
	2800 3900 2800 4500
Wire Wire Line
	2900 3800 2900 4150
Wire Wire Line
	6400 2850 6400 3000
$Comp
L VCC #PWR014
U 1 1 5BA267AA
P 3550 3550
F 0 "#PWR014" H 3550 3650 30  0001 C CNN
F 1 "VCC" H 3550 3650 30  0000 C CNN
F 2 "" H 3550 3550 60  0000 C CNN
F 3 "" H 3550 3550 60  0000 C CNN
	1    3550 3550
	1    0    0    -1  
$EndComp
Wire Wire Line
	2900 4150 4100 4150
Text Label 2550 3900 0    60   ~ 0
Yellow
Text Label 2550 3800 0    60   ~ 0
White
Text Label 2550 3700 0    60   ~ 0
Black
Text Label 2550 3600 0    60   ~ 0
Red
Wire Wire Line
	3750 3600 3750 3700
Wire Wire Line
	3750 4000 3750 4200
Wire Wire Line
	2550 3600 3750 3600
Wire Wire Line
	3400 3600 3400 3700
Wire Wire Line
	3400 4000 3400 4150
Connection ~ 3400 4150
Wire Wire Line
	3550 3550 3550 3600
Connection ~ 3550 3600
Connection ~ 3400 3600
Text Notes 1700 3450 0    60   ~ 12
CONTROLLARE ORDINE FILI
Wire Notes Line
	1750 3500 2850 3500
Wire Notes Line
	2850 3500 2850 4100
Wire Notes Line
	2850 4100 1750 4100
Wire Notes Line
	1750 4100 1750 3500
$Comp
L CONN_01X02 P2
U 1 1 5BB22981
P 1850 1250
F 0 "P2" H 1850 1400 50  0000 C CNN
F 1 "JBAT" V 1950 1250 50  0000 C CNN
F 2 "saet:molex_22-03-5025" H 1850 1250 50  0000 C CNN
F 3 "" H 1850 1250 50  0000 C CNN
F 4 "WK1011" H 1850 1250 60  0001 C CNN "Saet_code"
	1    1850 1250
	-1   0    0    1   
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR015
U 1 1 5BB61B60
P 3000 5050
F 0 "#PWR015" H 3000 5050 30  0001 C CNN
F 1 "GND" H 3000 4980 30  0001 C CNN
F 2 "" H 3000 5050 60  0000 C CNN
F 3 "" H 3000 5050 60  0000 C CNN
	1    3000 5050
	1    0    0    -1  
$EndComp
Wire Wire Line
	3000 4500 3000 4650
Connection ~ 3000 4500
Wire Notes Line
	3200 3400 4550 3400
Wire Notes Line
	4550 3400 4550 5200
Wire Notes Line
	4550 5200 3200 5200
Wire Notes Line
	3200 5200 3200 3400
Text Notes 3350 3400 0    60   ~ 12
CONNESSIONE SERIALE
Wire Notes Line
	3250 3700 3600 3700
Wire Notes Line
	3600 3700 3600 4250
Wire Notes Line
	3600 4250 3250 4250
Wire Notes Line
	3250 4250 3250 3700
Wire Notes Line
	4050 4000 4450 4000
Wire Notes Line
	4450 4000 4450 4300
Wire Notes Line
	4450 4300 4050 4300
Wire Notes Line
	4050 4300 4050 4000
Wire Notes Line
	2900 4600 3150 4600
Wire Notes Line
	3150 4600 3150 5150
Wire Notes Line
	3150 5150 2900 5150
Wire Notes Line
	2900 5150 2900 4600
Text Notes 4050 4000 0    21   ~ 4
CONNESSIONE CONTATTI
Text Notes 3250 3700 0    21   ~ 4
CONNESSIONE CONTATTI
Text Notes 2800 4600 0    21   ~ 4
CONNESSIONE CONTATTI	
$Comp
L CONN_01X02 P3
U 1 1 5BB628D6
P 2550 5700
F 0 "P3" H 2550 5850 50  0000 C CNN
F 1 "TAMPER CONNECTOR" V 2650 5700 50  0000 C CNN
F 2 "Connectors_Molex:Connector_Molex_PicoBlade_53047-0210" H 2550 5700 50  0000 C CNN
F 3 "" H 2550 5700 50  0000 C CNN
	1    2550 5700
	-1   0    0    1   
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR016
U 1 1 5BB62A9B
P 2800 5900
F 0 "#PWR016" H 2800 5900 30  0001 C CNN
F 1 "GND" H 2800 5830 30  0001 C CNN
F 2 "" H 2800 5900 60  0000 C CNN
F 3 "" H 2800 5900 60  0000 C CNN
	1    2800 5900
	1    0    0    -1  
$EndComp
Wire Wire Line
	2750 5750 2800 5750
Wire Wire Line
	2800 5750 2800 5900
Wire Wire Line
	2750 5650 6650 5650
Wire Wire Line
	6650 5650 6650 5200
$Comp
L R R5
U 1 1 5BB64289
P 3000 4800
F 0 "R5" V 3080 4800 50  0000 C CNN
F 1 "0" V 3000 4800 50  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 2930 4800 50  0000 C CNN
F 3 "" H 3000 4800 50  0000 C CNN
F 4 "RSA000 EXT" V 3000 4800 60  0001 C CNN "Saet_code"
	1    3000 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	3000 4950 3000 5050
Text Notes 850  5250 0    60   ~ 0
MONTAGGIO SERIALE:\n- R2 = 1k\n- R3 = 1k\n- R4 = 2.2M\n- R5 = NM\nMONTAGGIO CONTATTI:\n- R2 = 1k\n- R3 = 1k\n- R4 = NM\n- R5 = 0
Wire Notes Line
	750  4300 1900 4300
Wire Notes Line
	1900 4300 1900 5350
Wire Notes Line
	1900 5350 750  5350
Wire Notes Line
	750  5350 750  4300
$EndSCHEMATC
