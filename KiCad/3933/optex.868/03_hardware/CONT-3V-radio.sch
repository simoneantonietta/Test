EESchema Schematic File Version 2
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
LIBS:Contatto868-3V-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 2
Title "Magnetic Contact 3V on 868.3 MHz - CS 3933"
Date "mercoledì 03 giugno 2015"
Rev "2.0"
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text GLabel 10650 3550 2    60   Input ~ 0
CS
Text GLabel 10650 3700 2    60   Input ~ 0
CLK
Text GLabel 10650 3850 2    60   Input ~ 0
SI
Text GLabel 10650 4000 2    60   Input ~ 0
SO
Text GLabel 10650 4150 2    60   Input ~ 0
IRQ
Text GLabel 10650 4300 2    60   Input ~ 0
PD
NoConn ~ 8800 4150
NoConn ~ 7950 4700
NoConn ~ 7500 4150
$Comp
L VCC #PWR017
U 1 1 53F4AF5F
P 9350 4150
F 0 "#PWR017" H 9350 4250 30  0001 C CNN
F 1 "VCC" H 9350 4250 30  0000 C CNN
F 2 "" H 9350 4150 60  0000 C CNN
F 3 "" H 9350 4150 60  0000 C CNN
	1    9350 4150
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C17
U 1 1 53F4AF6E
P 9150 4550
AR Path="/53F4AF6E" Ref="C17"  Part="1" 
AR Path="/53F4C1B5/53F4AF6E" Ref="C17"  Part="1" 
F 0 "C17" H 9200 4650 59  0000 L CNN
F 1 "100n" H 9200 4450 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 9188 4400 30  0001 C CNN
F 3 "" H 9150 4550 60  0000 C CNN
F 4 "CSA104 EXT" H 9150 4550 60  0001 C CNN "Saet_code"
	1    9150 4550
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR018
U 1 1 53F4AF7D
P 9150 4850
F 0 "#PWR018" H 9150 4850 30  0001 C CNN
F 1 "GND" H 9150 4780 30  0001 C CNN
F 2 "" H 9150 4850 60  0000 C CNN
F 3 "" H 9150 4850 60  0000 C CNN
	1    9150 4850
	1    0    0    -1  
$EndComp
$Comp
L CRYSTAL QZ1
U 1 1 53F4AFDE
P 8100 2400
F 0 "QZ1" H 8100 2550 59  0000 C CNN
F 1 "30MHz" H 8100 2200 59  0000 C CNN
F 2 "Crystals:Crystal_SMD_2520-4pin_2.5x2.0mm" H 8100 2400 60  0001 C CNN
F 3 "~" H 8100 2400 60  0000 C CNN
F 4 "YQ0306S" H 8100 2400 60  0001 C CNN "Saet_code"
	1    8100 2400
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C8
U 1 1 53F4B2DA
P 8350 5600
AR Path="/53F4B2DA" Ref="C8"  Part="1" 
AR Path="/53F4C1B5/53F4B2DA" Ref="C8"  Part="1" 
F 0 "C8" H 8400 5700 59  0000 L CNN
F 1 "100n" H 8400 5500 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 8388 5450 30  0001 C CNN
F 3 "~" H 8350 5600 60  0000 C CNN
F 4 "CSA104 EXT" H 8350 5600 60  0001 C CNN "Saet_code"
	1    8350 5600
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C6
U 1 1 53F4B2E7
P 9000 5600
AR Path="/53F4B2E7" Ref="C6"  Part="1" 
AR Path="/53F4C1B5/53F4B2E7" Ref="C6"  Part="1" 
F 0 "C6" H 9050 5700 59  0000 L CNN
F 1 "1u" H 9050 5500 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 9038 5450 30  0001 C CNN
F 3 "~" H 9000 5600 60  0000 C CNN
F 4 "CSA105 EXT" H 9000 5600 60  0001 C CNN "Saet_code"
	1    9000 5600
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR019
U 1 1 53F4B2EF
P 9000 6100
F 0 "#PWR019" H 9000 6100 30  0001 C CNN
F 1 "GND" H 9000 6030 30  0001 C CNN
F 2 "" H 9000 6100 60  0000 C CNN
F 3 "" H 9000 6100 60  0000 C CNN
	1    9000 6100
	1    0    0    -1  
$EndComp
$Comp
L VCC #PWR020
U 1 1 53F4B52A
P 3700 1100
F 0 "#PWR020" H 3700 1200 30  0001 C CNN
F 1 "VCC" H 3700 1200 30  0000 C CNN
F 2 "" H 3700 1100 60  0000 C CNN
F 3 "" H 3700 1100 60  0000 C CNN
	1    3700 1100
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C7
U 1 1 53F4B539
P 3700 1600
AR Path="/53F4B539" Ref="C7"  Part="1" 
AR Path="/53F4C1B5/53F4B539" Ref="C7"  Part="1" 
F 0 "C7" H 3700 1700 59  0000 L CNN
F 1 "2u2" H 3700 1500 59  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 3738 1450 30  0001 C CNN
F 3 "~" H 3700 1600 60  0000 C CNN
F 4 "CS1225 EXT" H 3700 1600 60  0001 C CNN "Saet_code"
	1    3700 1600
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C15
U 1 1 53F4B546
P 4150 1600
AR Path="/53F4B546" Ref="C15"  Part="1" 
AR Path="/53F4C1B5/53F4B546" Ref="C15"  Part="1" 
F 0 "C15" H 4150 1700 59  0000 L CNN
F 1 "100n" H 4150 1500 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 4188 1450 30  0001 C CNN
F 3 "~" H 4150 1600 60  0000 C CNN
F 4 "CSA104 EXT" H 4150 1600 60  0001 C CNN "Saet_code"
	1    4150 1600
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C11
U 1 1 53F4B54C
P 4600 1600
AR Path="/53F4B54C" Ref="C11"  Part="1" 
AR Path="/53F4C1B5/53F4B54C" Ref="C11"  Part="1" 
F 0 "C11" H 4600 1700 59  0000 L CNN
F 1 "100p" H 4600 1500 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 4638 1450 30  0001 C CNN
F 3 "~" H 4600 1600 60  0000 C CNN
F 4 "CSA101 EXT" H 4600 1600 60  0001 C CNN "Saet_code"
	1    4600 1600
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C10
U 1 1 53F4B552
P 5050 1600
AR Path="/53F4B552" Ref="C10"  Part="1" 
AR Path="/53F4C1B5/53F4B552" Ref="C10"  Part="1" 
F 0 "C10" H 5050 1700 59  0000 L CNN
F 1 "33p" H 5050 1500 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 5088 1450 30  0001 C CNN
F 3 "~" H 5050 1600 60  0000 C CNN
F 4 "CSA330 EXT" H 5050 1600 60  0001 C CNN "Saet_code"
	1    5050 1600
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR021
U 1 1 53F4B55A
P 5050 2100
F 0 "#PWR021" H 5050 2100 30  0001 C CNN
F 1 "GND" H 5050 2030 30  0001 C CNN
F 2 "" H 5050 2100 60  0000 C CNN
F 3 "" H 5050 2100 60  0000 C CNN
	1    5050 2100
	1    0    0    -1  
$EndComp
$Comp
L R-RESCUE-Contatto868-3V R15
U 1 1 53F4B811
P 6300 1600
F 0 "R15" V 6400 1600 59  0000 C CNN
F 1 "0" V 6307 1601 55  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 6230 1600 30  0001 C CNN
F 3 "~" H 6300 1600 30  0000 C CNN
F 4 "RSA000 EXT" V 6300 1600 60  0001 C CNN "Saet_code"
	1    6300 1600
	1    0    0    -1  
$EndComp
$Comp
L INDUCTOR L1
U 1 1 53F4B986
P 6300 2350
F 0 "L1" H 6300 2500 59  0000 C CNN
F 1 "120n" H 6300 2250 59  0000 C CNN
F 2 "Inductors_SMD:L_0402" H 6300 2350 60  0001 C CNN
F 3 "~" H 6300 2350 60  0000 C CNN
F 4 "TB3103 EXT" H 6300 2350 60  0001 C CNN "Saet_code"
	1    6300 2350
	0    -1   -1   0   
$EndComp
$Comp
L INDUCTOR L11
U 1 1 53F4B9A9
P 5500 3500
F 0 "L11" H 5500 3650 59  0000 C CNN
F 1 "5n6" H 5500 3400 59  0000 C CNN
F 2 "Inductors_SMD:L_0402" H 5500 3500 60  0001 C CNN
F 3 "~" H 5500 3500 60  0000 C CNN
F 4 "TB3106 EXT" H 5500 3500 60  0001 C CNN "Saet_code"
	1    5500 3500
	0    -1   1    0   
$EndComp
$Comp
L INDUCTOR L5
U 1 1 53F4B9AF
P 4500 3100
F 0 "L5" H 4500 3250 59  0000 C CNN
F 1 "15n" H 4500 3000 59  0000 C CNN
F 2 "Inductors_SMD:L_0402" H 4500 3100 60  0001 C CNN
F 3 "~" H 4500 3100 60  0000 C CNN
F 4 "TB3104 EXT" H 4500 3100 60  0001 C CNN "Saet_code"
	1    4500 3100
	1    0    0    -1  
$EndComp
$Comp
L INDUCTOR L4
U 1 1 53F4B9C4
P 3600 3100
F 0 "L4" H 3600 3250 59  0000 C CNN
F 1 "15n" H 3600 3000 59  0000 C CNN
F 2 "Inductors_SMD:L_0402" H 3600 3100 60  0001 C CNN
F 3 "~" H 3600 3100 60  0000 C CNN
F 4 "TB3104 EXT" H 3600 3100 60  0001 C CNN "Saet_code"
	1    3600 3100
	1    0    0    -1  
$EndComp
$Comp
L INDUCTOR L2
U 1 1 53F4BAF9
P 6800 4800
F 0 "L2" H 6800 4950 59  0000 C CNN
F 1 "11n" H 6800 4700 59  0000 C CNN
F 2 "Inductors_SMD:L_0402" H 6800 4800 60  0001 C CNN
F 3 "~" H 6800 4800 60  0000 C CNN
F 4 "TB3102 EXT" H 6800 4800 60  0001 C CNN "Saet_code"
	1    6800 4800
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C18
U 1 1 53F4BB9F
P 5900 5200
AR Path="/53F4BB9F" Ref="C18"  Part="1" 
AR Path="/53F4C1B5/53F4BB9F" Ref="C18"  Part="1" 
F 0 "C18" H 5950 5300 59  0000 L CNN
F 1 "6p8" H 5950 5100 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 5938 5050 30  0001 C CNN
F 3 "~" H 5900 5200 60  0000 C CNN
F 4 "CSA6P8 EXT" H 5900 5200 60  0001 C CNN "Saet_code"
	1    5900 5200
	0    -1   -1   0   
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C12
U 1 1 53F4BC4C
P 5900 3100
AR Path="/53F4BC4C" Ref="C12"  Part="1" 
AR Path="/53F4C1B5/53F4BC4C" Ref="C12"  Part="1" 
F 0 "C12" H 5950 3200 59  0000 L CNN
F 1 "39p" H 5950 3000 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 5938 2950 30  0001 C CNN
F 3 "~" H 5900 3100 60  0000 C CNN
F 4 "CSA390 EXT" H 5900 3100 60  0001 C CNN "Saet_code"
	1    5900 3100
	0    -1   -1   0   
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C14
U 1 1 53F4BD1B
P 5000 3500
AR Path="/53F4BD1B" Ref="C14"  Part="1" 
AR Path="/53F4C1B5/53F4BD1B" Ref="C14"  Part="1" 
F 0 "C14" H 5050 3600 59  0000 L CNN
F 1 "6p" H 5050 3400 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 5038 3350 30  0001 C CNN
F 3 "~" H 5000 3500 60  0000 C CNN
F 4 "CSA6P0 EXT" H 5000 3500 60  0001 C CNN "Saet_code"
	1    5000 3500
	1    0    0    -1  
$EndComp
$Comp
L R-RESCUE-Contatto868-3V R1
U 1 1 53F4BF2B
P 5000 4350
F 0 "R1" V 5100 4350 59  0000 C CNN
F 1 "50" V 5007 4351 55  0000 C CNN
F 2 "Resistors_SMD:R_0402" V 4930 4350 30  0000 C CNN
F 3 "~" H 5000 4350 30  0000 C CNN
F 4 "RSA500 EXT" V 5000 4350 60  0001 C CNN "Saet_code"
	1    5000 4350
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR022
U 1 1 53F4C0C8
P 5500 4950
F 0 "#PWR022" H 5500 4950 30  0001 C CNN
F 1 "GND" H 5500 4880 30  0001 C CNN
F 2 "" H 5500 4950 60  0000 C CNN
F 3 "" H 5500 4950 60  0000 C CNN
	1    5500 4950
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C13
U 1 1 53F4C14C
P 4050 3500
AR Path="/53F4C14C" Ref="C13"  Part="1" 
AR Path="/53F4C1B5/53F4C14C" Ref="C13"  Part="1" 
F 0 "C13" H 4100 3600 59  0000 L CNN
F 1 "3p3" H 4100 3400 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 4088 3350 30  0001 C CNN
F 3 "~" H 4050 3500 60  0000 C CNN
F 4 "CSA3P3 EXT" H 4050 3500 60  0001 C CNN "Saet_code"
	1    4050 3500
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C20
U 1 1 53F4C152
P 3200 3500
AR Path="/53F4C152" Ref="C20"  Part="1" 
AR Path="/53F4C1B5/53F4C152" Ref="C20"  Part="1" 
F 0 "C20" H 3250 3600 59  0000 L CNN
F 1 "68p" H 3250 3400 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 3238 3350 30  0001 C CNN
F 3 "~" H 3200 3500 60  0000 C CNN
F 4 "CSA680 EXT" H 3200 3500 60  0001 C CNN "Saet_code"
	1    3200 3500
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR023
U 1 1 53F4C35A
P 4050 3900
F 0 "#PWR023" H 4050 3900 30  0001 C CNN
F 1 "GND" H 4050 3830 30  0001 C CNN
F 2 "" H 4050 3900 60  0000 C CNN
F 3 "" H 4050 3900 60  0000 C CNN
	1    4050 3900
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C5
U 1 1 53F4C69B
P 2750 6600
AR Path="/53F4C69B" Ref="C5"  Part="1" 
AR Path="/53F4C1B5/53F4C69B" Ref="C5"  Part="1" 
F 0 "C5" H 2800 6700 59  0000 L CNN
F 1 "1p" H 2800 6500 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 2788 6450 30  0001 C CNN
F 3 "~" H 2750 6600 60  0000 C CNN
F 4 "CSA1P0 EXT" H 2750 6600 60  0001 C CNN "Saet_code"
	1    2750 6600
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C19
U 1 1 53F4C6A1
P 3100 6650
AR Path="/53F4C6A1" Ref="C19"  Part="1" 
AR Path="/53F4C1B5/53F4C6A1" Ref="C19"  Part="1" 
F 0 "C19" H 3150 6750 59  0000 L CNN
F 1 "1p" H 3150 6550 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 3138 6500 30  0001 C CNN
F 3 "~" H 3100 6650 60  0000 C CNN
F 4 "CSA1P0 EXT" H 3100 6650 60  0001 C CNN "Saet_code"
	1    3100 6650
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR024
U 1 1 53F4C811
P 2750 7100
F 0 "#PWR024" H 2750 7100 30  0001 C CNN
F 1 "GND" H 2750 7030 30  0001 C CNN
F 2 "" H 2750 7100 60  0000 C CNN
F 3 "" H 2750 7100 60  0000 C CNN
	1    2750 7100
	1    0    0    -1  
$EndComp
$Comp
L INDUCTOR L3
U 1 1 53F4CB1F
P 2000 2900
F 0 "L3" H 2000 3050 59  0000 C CNN
F 1 "9n" H 2000 2800 59  0000 C CNN
F 2 "Inductors_SMD:L_0402" H 2000 2900 60  0001 C CNN
F 3 "~" H 2000 2900 60  0000 C CNN
F 4 "TB3105 EXT" H 2000 2900 60  0001 C CNN "Saet_code"
	1    2000 2900
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C27
U 1 1 53F4CB25
P 1400 3250
AR Path="/53F4CB25" Ref="C27"  Part="1" 
AR Path="/53F4C1B5/53F4CB25" Ref="C27"  Part="1" 
F 0 "C27" H 1450 3350 59  0000 L CNN
F 1 "4p7" H 1450 3150 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 1438 3100 30  0001 C CNN
F 3 "~" H 1400 3250 60  0000 C CNN
F 4 "CSA1P0 EXT" H 1400 3250 60  0001 C CNN "Saet_code"
	1    1400 3250
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR025
U 1 1 53F4CB2B
P 1400 3600
F 0 "#PWR025" H 1400 3600 30  0001 C CNN
F 1 "GND" H 1400 3530 30  0001 C CNN
F 2 "" H 1400 3600 60  0000 C CNN
F 3 "" H 1400 3600 60  0000 C CNN
	1    1400 3600
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C1
U 1 1 53F4CB37
P 3100 5050
AR Path="/53F4CB37" Ref="C1"  Part="1" 
AR Path="/53F4C1B5/53F4CB37" Ref="C1"  Part="1" 
F 0 "C1" H 3150 5150 59  0000 L CNN
F 1 "68p" H 3150 4950 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 3138 4900 30  0001 C CNN
F 3 "~" H 3100 5050 60  0000 C CNN
F 4 "CSA680 EXT" H 3100 5050 60  0001 C CNN "Saet_code"
	1    3100 5050
	0    -1   -1   0   
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C24
U 1 1 53F4CB43
P 2650 5400
AR Path="/53F4CB43" Ref="C24"  Part="1" 
AR Path="/53F4C1B5/53F4CB43" Ref="C24"  Part="1" 
F 0 "C24" H 2700 5500 51  0000 L CNN
F 1 "4p7" H 2700 5300 51  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 2688 5250 30  0001 C CNN
F 3 "~" H 2650 5400 60  0000 C CNN
F 4 "CSA4P7 EXT" H 2650 5400 60  0001 C CNN "Saet_code"
	1    2650 5400
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR026
U 1 1 53F4CB4F
P 2650 5750
F 0 "#PWR026" H 2650 5750 30  0001 C CNN
F 1 "GND" H 2650 5680 30  0001 C CNN
F 2 "" H 2650 5750 60  0000 C CNN
F 3 "" H 2650 5750 60  0000 C CNN
	1    2650 5750
	1    0    0    -1  
$EndComp
$Comp
L UPG2214TB U5
U 1 1 53F5B67A
P 4200 5000
F 0 "U5" H 3800 5350 59  0000 C CNN
F 1 "UPG2214TB" H 4350 5350 59  0000 C CNN
F 2 "TO_SOT_Packages_SMD:SOT-363_SC-70-6" H 4200 5000 60  0001 C CNN
F 3 "~" H 4200 5000 60  0000 C CNN
F 4 "IL1196S" H 4200 5000 60  0001 C CNN "Saet_code"
	1    4200 5000
	1    0    0    -1  
$EndComp
$Comp
L SI4432B U1
U 1 1 53F5B695
P 8150 3950
F 0 "U1" H 7750 4550 60  0000 C CNN
F 1 "SI4432B" H 8650 4550 59  0000 C CNN
F 2 "Housings_DFN_QFN:QFN-20-1EP_4x4mm_Pitch0.5mm" H 8150 3950 60  0001 C CNN
F 3 "~" H 8150 3950 60  0000 C CNN
F 4 "IU1162S" H 8150 3950 60  0001 C CNN "Saet_code"
	1    8150 3950
	1    0    0    -1  
$EndComp
Text Notes 10200 3550 0    39   ~ 0
U2(6)
Text Notes 10200 3700 0    39   ~ 0
U2(31)
Text Notes 10200 3850 0    39   ~ 0
U2(22)
Text Notes 10200 4000 0    39   ~ 0
U2(21)
Text Notes 10200 4150 0    39   ~ 0
U2(17)
Text Notes 10200 4300 0    39   ~ 0
U2(20)
NoConn ~ 8250 4700
$Comp
L ANTENNA_POL ANT1
U 1 1 546215C2
P 1400 2650
F 0 "ANT1" H 1500 2890 59  0000 R CNN
F 1 "Antenna Johanson" V 1200 2450 59  0000 L CNN
F 2 "saet:antenna_Johanson" H 1400 2650 60  0001 C CNN
F 3 "" H 1400 2650 60  0000 C CNN
F 4 "US5602" H 1400 2650 60  0001 C CNN "Saet_code"
	1    1400 2650
	1    0    0    -1  
$EndComp
$Comp
L C-RESCUE-Contatto868-3V C9
U 1 1 54626D46
P 7300 5150
AR Path="/54626D46" Ref="C9"  Part="1" 
AR Path="/53F4C1B5/54626D46" Ref="C9"  Part="1" 
F 0 "C9" H 7350 5250 59  0000 L CNN
F 1 "3p9" H 7350 5050 59  0000 L CNN
F 2 "Capacitors_SMD:C_0402" H 7338 5000 30  0001 C CNN
F 3 "" H 7300 5150 60  0000 C CNN
F 4 "CSA3P9 EXT" H 7300 5150 60  0001 C CNN "Saet_code"
	1    7300 5150
	1    0    0    -1  
$EndComp
$Comp
L GND-RESCUE-Contatto868-3V #PWR027
U 1 1 54626F2C
P 7300 5450
F 0 "#PWR027" H 7300 5450 30  0001 C CNN
F 1 "GND" H 7300 5380 30  0001 C CNN
F 2 "" H 7300 5450 60  0000 C CNN
F 3 "" H 7300 5450 60  0000 C CNN
	1    7300 5450
	1    0    0    -1  
$EndComp
Wire Wire Line
	9800 3550 10650 3550
Wire Wire Line
	10650 3700 10100 3700
Wire Wire Line
	8800 3850 10650 3850
Wire Wire Line
	10650 4000 10100 4000
Wire Wire Line
	8800 3750 10100 3750
Wire Wire Line
	10100 3750 10100 3700
Wire Wire Line
	8800 3950 10100 3950
Wire Wire Line
	10100 3950 10100 4000
Wire Wire Line
	9150 4750 9150 4850
Wire Wire Line
	8800 4050 9150 4050
Wire Wire Line
	9150 4050 9150 4350
Connection ~ 9150 4200
Wire Wire Line
	8150 2700 8150 3200
Wire Wire Line
	8150 2700 8450 2700
Wire Wire Line
	8450 2700 8450 2400
Wire Wire Line
	7750 2400 7750 2700
Wire Wire Line
	7750 2700 8050 2700
Wire Wire Line
	8050 2700 8050 3200
Wire Wire Line
	8350 2900 8350 3200
Wire Wire Line
	8250 2800 8250 3200
Wire Wire Line
	8350 2900 9800 2900
Wire Wire Line
	9800 2900 9800 3550
Wire Wire Line
	8250 2800 9700 2800
Wire Wire Line
	9700 2800 9700 4150
Wire Wire Line
	9700 4150 10650 4150
Wire Wire Line
	7950 2800 7950 3200
Wire Wire Line
	7950 2800 7650 2800
Wire Wire Line
	7650 2800 7650 2150
Wire Wire Line
	7650 2150 9600 2150
Wire Wire Line
	9600 2150 9600 4300
Wire Wire Line
	9600 4300 10650 4300
Wire Wire Line
	8350 4700 8350 5400
Wire Wire Line
	8350 5800 8350 5900
Wire Wire Line
	8350 5900 9000 5900
Wire Wire Line
	9000 5800 9000 6100
Connection ~ 9000 5900
Wire Wire Line
	9000 5400 9000 5300
Wire Wire Line
	9000 5300 8350 5300
Connection ~ 8350 5300
Wire Wire Line
	3700 1200 7000 1200
Wire Wire Line
	7000 1200 7000 3750
Connection ~ 3700 1200
Wire Wire Line
	4150 1400 4150 1200
Connection ~ 4150 1200
Wire Wire Line
	4600 1400 4600 1200
Connection ~ 4600 1200
Wire Wire Line
	5050 1400 5050 1200
Connection ~ 5050 1200
Wire Wire Line
	3700 1800 3700 2000
Wire Wire Line
	3700 2000 5050 2000
Wire Wire Line
	5050 1800 5050 2100
Wire Wire Line
	4600 1800 4600 2000
Connection ~ 4600 2000
Wire Wire Line
	4150 1800 4150 2000
Connection ~ 4150 2000
Connection ~ 5050 2000
Connection ~ 6300 1200
Wire Wire Line
	6300 1850 6300 2100
Wire Wire Line
	6300 2600 6300 3850
Wire Wire Line
	6300 3850 7500 3850
Wire Wire Line
	7500 3950 6300 3950
Wire Wire Line
	6300 3950 6300 5200
Wire Wire Line
	6300 4800 6550 4800
Connection ~ 6300 4800
Wire Wire Line
	7500 4050 7300 4050
Wire Wire Line
	7300 4050 7300 4950
Wire Wire Line
	7050 4800 7300 4800
Wire Wire Line
	6300 5200 6100 5200
Wire Wire Line
	5700 5200 4650 5200
Wire Wire Line
	6100 3100 6300 3100
Connection ~ 6300 3100
Wire Wire Line
	4750 3100 5700 3100
Wire Wire Line
	5000 3100 5000 3300
Wire Wire Line
	5500 3100 5500 3250
Connection ~ 5500 3100
Wire Wire Line
	5000 4600 5000 5050
Wire Wire Line
	5000 3700 5000 4100
Connection ~ 5000 3900
Wire Wire Line
	5000 5050 4650 5050
Wire Wire Line
	5000 4800 5500 4800
Wire Wire Line
	5500 4800 5500 4950
Connection ~ 5000 4800
Connection ~ 5000 3100
Wire Wire Line
	3850 3100 4250 3100
Wire Wire Line
	4050 3300 4050 3100
Connection ~ 4050 3100
Wire Wire Line
	3200 3300 3200 3100
Wire Wire Line
	3200 3100 3350 3100
Wire Wire Line
	3200 3700 3200 4100
Wire Wire Line
	3200 4100 4750 4100
Wire Wire Line
	4750 4100 4750 4900
Wire Wire Line
	4750 4900 4650 4900
Wire Wire Line
	4050 3700 4050 3900
Wire Wire Line
	3700 5200 3700 6300
Wire Wire Line
	2750 6300 8050 6300
Wire Wire Line
	8050 6300 8050 4700
Wire Wire Line
	8150 6400 8150 4700
Wire Wire Line
	3100 6400 8150 6400
Wire Wire Line
	3600 4900 3600 6400
Wire Wire Line
	3600 4900 3700 4900
Connection ~ 3700 6300
Wire Wire Line
	3100 6400 3100 6450
Connection ~ 3600 6400
Wire Wire Line
	2750 6800 2750 7100
Wire Wire Line
	3100 6850 3100 6900
Wire Wire Line
	3100 6900 2750 6900
Connection ~ 2750 6900
Wire Wire Line
	2650 5050 2900 5050
Wire Wire Line
	2650 5750 2650 5600
Wire Wire Line
	3300 5050 3700 5050
Wire Wire Line
	2650 2900 2650 5200
Wire Wire Line
	2250 2900 2650 2900
Wire Wire Line
	1400 2750 1400 3050
Wire Wire Line
	1400 2900 1750 2900
Wire Wire Line
	1400 3450 1400 3600
Connection ~ 1400 2900
Wire Wire Line
	9150 4200 9350 4200
Wire Wire Line
	9350 4200 9350 4150
Wire Wire Line
	6300 1350 6300 1200
Connection ~ 2650 5050
Wire Wire Line
	3700 1100 3700 1400
Wire Wire Line
	7750 2400 7800 2400
Wire Wire Line
	8450 2400 8400 2400
Connection ~ 7300 4800
Wire Wire Line
	7300 5350 7300 5450
Wire Wire Line
	5500 3750 5500 3900
Wire Wire Line
	5500 3900 5000 3900
Wire Wire Line
	2750 6300 2750 6400
Wire Wire Line
	7000 3750 7500 3750
$Comp
L GND-RESCUE-Contatto868-3V #PWR028
U 1 1 5C0A69F6
P 7500 4350
F 0 "#PWR028" H 7500 4350 30  0001 C CNN
F 1 "GND" H 7500 4280 30  0001 C CNN
F 2 "" H 7500 4350 60  0000 C CNN
F 3 "" H 7500 4350 60  0000 C CNN
	1    7500 4350
	1    0    0    -1  
$EndComp
Wire Wire Line
	7500 4300 7500 4350
$EndSCHEMATC
