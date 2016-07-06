solarmon
========

Monitoring for JFY solar inverter written in C for linux.

This is a simple program to read the inverter data from a JFY grid connected solar inverter.

This program was inspired by solarmonj by Adam Gray and John Croucher https://code.google.com/p/solarmonj/ and created to provide some different configuration and logging options. I have added some command line options to change the way the system connects to a serial port and logs the collected data. These modifications could also easily be added to the solarmonj program.

To build the application under Linux or OSX

```
pi@raspberrypi ~ $ make 
pi@raspberrypi ~ $ sudo cp solarmon /usr/bin
```

To test the application you can run the following

```
pi@raspberrypi ~ $ solarmon -d -p /dev
Failed to open serial port.

pi@raspberrypi ~ $
```

Then if you know which serial port you are using
```
pi@raspberrypi ~ $ solarmon -d -p /dev/ttyS0
==> A5A50100304400FE410A0D
==> A5A50100304000FE450A0D
Failed to register inverter.

pi@raspberrypi ~ $
```

And finally if you have the inverter attached to your system
```
pi@raspberrypi ~ $ ./solarmon -d -p /dev/ttyUSB0
==> A5A50100304400FE410A0D
==> A5A50100304000FE450A0D
<== A5A5000030BF1031353133313331323130313438202020FAC80A0D
==> A5A501003041113135313331333132313031343820202001FB430A0D
<== A5A5010130BE0106FDBF0A0D
==> A5A50101314200FE410A0D
<== A5A5010131BD2A01D608B9084E000C0973138C00C8FFFF00005EF9000005E4000100000000000000000000000000000000F5800A0D
temp:47.000000 TodayE:22.330000 VDC:212.600006 I:1.200000 VAC:241.899994 Freq:50.040001 CurrE:200.000000 unk1:65535.000000 unk2:0.000000 TotE:2431.300049

pi@raspberrypi ~ $
```

You can then add the program to your system's cron to run every minute appending to a file, for example:
```
pi@raspberrypi ~ $ sudo -i
pi@raspberrypi ~ # echo "# Run solarmon every minute" > /etc/cron.d/solarmon
pi@raspberrypi ~ # echo "* * * * * root /usr/bin/solarmon -p /dev/ttyUSB0 >> /tmp/solarmon-out" >> /etc/cron.d/solarmon
pi@raspberrypi ~ # exit
pi@raspberrypi ~ $
```
or this line to add to the system logs
```
pi@raspberrypi ~ # echo "* * * * * root /usr/bin/solarmon -p /dev/ttyUSB0 | /usr/bin/logger" >> /etc/cron.d/solarmon
```
