solarmon
========

Monitoring for JFY solar inverter written in C for linux.

This is a simple program to read the inverter data from a JFY grid connected solar inverter.

This program was inspired by solarmonj by Adam Gray and John Croucher https://code.google.com/p/solarmonj/

To build the application under Linux or OSX

```
$> gcc solarmon.c -o solarmon
$> sudo cp solarmon /usr/bin
```

To test the application you can run the following

```
$> solarmon -d -p /dev
Failed to open serial port.
$> 
```

Then if you know which serial port you are using
```
$> solarmon -d -p /dev/ttyS0
==> A5A50100304400FE410A0D
==> A5A50100304000FE450A0D
Failed to register inverter.
$>
```

And finally if you have the inverter attached to your system
```
$> solarmon -d -p /dev/ttyUSB0

$>
```

