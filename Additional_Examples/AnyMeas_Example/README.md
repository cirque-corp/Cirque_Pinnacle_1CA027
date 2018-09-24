# AnyMeas - Pinnacle

// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

### Purpose

The purpose of the AnyMeas_Example.ino program is to show how to access and use the AnyMeas mode of the Pinnacle device. In AnyMeas mode, the
hardware reports raw ADC data. It is the lowest level of device configuration
available on from the Touchpad. This differs from the default operation
(Absolute Mode: data represents x, y and z positions detected by the touchpad)
which calculates the reported position from multiple measurements.

### AnyMeas Uses

The touchpad is designed by using a grid of electrodes. In AnyMeas mode each
each axis position can be activated individually. This mode gives developers
the greatest freedom of device configuration and measurement.

Custom compensation matrices can be quickly configured and tested, along with other application specific measurements and configurations. 

### Sample Program Output

    Initial Test
    Meas 0: -27	Meas 1: 3	Meas 2: -22	Meas 3: -25	Meas 4: 15
    Meas 0: -20	Meas 1: -5	Meas 2: -22	Meas 3: 23	Meas 4: 7
    Meas 0: 37	Meas 1: -27	Meas 2: 12	Meas 3: 9	Meas 4: 4
    Meas 0: 31	Meas 1: 33	Meas 2: 5	Meas 3: -8	Meas 4: -44
    Meas 0: 24	Meas 1: 14	Meas 2: -12	Meas 3: 7	Meas 4: -43
    Meas 0: -16	Meas 1: -70	Meas 2: -3	Meas 3: 30	Meas 4: -14
    Meas 0: -20	Meas 1: -19	Meas 2: 8	Meas 3: -7	Meas 4: -34
    Meas 0: -27	Meas 1: -19	Meas 2: -15	Meas 3: -27	Meas 4: -47
    Meas 0: 21	Meas 1: -14	Meas 2: 34	Meas 3: 65	Meas 4: -18
    Meas 0: 43	Meas 1: -55	Meas 2: -19	Meas 3: 1	Meas 4: 45
    Meas 0: 25	Meas 1: -4	Meas 2: 30	Meas 3: 23	Meas 4: 27
