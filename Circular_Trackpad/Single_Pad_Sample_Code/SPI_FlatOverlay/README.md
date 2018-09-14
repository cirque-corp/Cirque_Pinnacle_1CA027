// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

This directory contains a sample program for evaluating the TM0XX0XX (SPI version) with a Teensy 3.2 Arduino development board. This program communicates with USB CDC to provide
formatted data from the touchpad to the Host.

Example output from serial terminal:

  Pinnacle Initialized...

  Setting ADC gain...
  ADC gain set to:	40 (X/2)

  Setting xAxis.WideZMin...
  Current value:	6
  New value:	4

  Setting yAxis.WideZMin...
  Current value:	5
  New value:	3

  X	    Y	Z	Data
  1501	470	21	valid
  1492	476	21	valid
  1481	481	21	valid
  1470	487	21	valid
  1459	496	21	valid
  1447	504	23	valid
  1434	510	21	valid
  1421	517	23	valid
  1407	524	21	valid
  1393	530	19	valid
  1377	534	18	valid
  1355	540	17	valid
  1328	548	16	valid
  1294	559	13	valid
  1253	572	14	hovering
  1212	582	13	hovering
  1171	591	12	hovering
  1136	597	10	hovering
  1113	604	6	hovering
  1095	610	5	hovering
  0	0	0	liftoff
  0	0	0	liftoff
  0	0	0	liftoff
  0	0	0	liftoff
