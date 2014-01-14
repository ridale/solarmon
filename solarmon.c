/**
    solarmon a command line tool to poll a JFY grid connect solar inverter
    Copyright (C) 2014  Richard Lemon

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

int debug = 0;
unsigned char outbuffer[256];
unsigned char inbuffer[256];
const unsigned char sourceaddr = 1;
unsigned char destaddr = 0;
const int headerlen = 7;
char *serial_device = "/dev/ttyS0";
char *log_path = "/var/log/solarmonj/";

/**
\brief	Log to console or syslog.
\author rdl 
*/
void log_output(int priority, char* message)
{
	if (debug) {
		if (priority == LOG_ERR)
			fprintf(stderr, "%s\n", message);
		else
			printf("%s\n", message);
	}
	else
		syslog(priority, "%s", message);
}

/**
\brief  Debug method to print buffer as hex string.
\author rdl 
*/
void dumphex(unsigned char* buf, int buflen, int out)
{
    if (buflen < 1) return;
    if (out)    printf("==> ");
    else        printf("<== ");
    int i;
    for (i = 0; i < buflen; i++)
    {
        //if (i > 0) printf(":");
        printf("%02X", buf[i]);
    }
    printf("\n");
}

/**
\brief  Calculate sum as per JFY
\author rdl 
*/
unsigned short checksum(int length)
{
    unsigned short sum = 0;
    int idx;
    for (idx = 0; idx < length; idx++) {
        sum += outbuffer[idx];
    }
    sum ^= 0xffff;
    sum += 1;
    return sum;
}
     
/**
\brief  Creates a JFY inverter command.
\author rdl 
*/
int create_command(char ctl, char func, unsigned char* data, int len)
{
    int cmdlength = len;
    // the command cannot be greater than max unsigned byte minus overhead
    if (cmdlength > 240 ) return 0;
        
    outbuffer[0] = 0xa5;
    outbuffer[1] = 0xa5;
    outbuffer[2] = sourceaddr;
    outbuffer[3] = destaddr;
    outbuffer[4] = ctl;
    outbuffer[5] = func;
    outbuffer[6] = cmdlength;
    cmdlength += headerlen;
    memcpy(&outbuffer[headerlen], data, outbuffer[6]);
    unsigned short sum = checksum(cmdlength);
    outbuffer[cmdlength++] = (sum & 0xff00) >> 8;
    outbuffer[cmdlength++] = (sum & 0x00ff);
    outbuffer[cmdlength++] = '\n';
    outbuffer[cmdlength++] = '\r';
    if (debug)
        dumphex(outbuffer, cmdlength, 1);

    return cmdlength;
}

/**
\brief  Sets up the serial port for JFY comms.
\author rdl 
*/
int open_serial(char* serial_device)
{
    int serfd = open(serial_device, O_RDWR | O_NOCTTY | O_NDELAY);
    if ( serfd < 0) return serfd;

    fcntl( serfd, F_SETFL, FNDELAY);

	// Set the serial options for JFY comms
    struct termios options;
    tcgetattr( serfd, &options );
    cfsetispeed( &options, B9600 );
    cfsetospeed( &options, B9600 );
    cfmakeraw( &options );
    options.c_cc[ VMIN ] = 0;
    options.c_cc[ VTIME ] = 10;
    tcsetattr( serfd, TCSANOW, &options );

    return serfd;
}

/**
\brief  Write to the serial port
\author rdl 
*/
int write_serial(int serfd, int cmdlength)
{
    return write(serfd, outbuffer, cmdlength);
}

/**
\brief  Read from the serial port
\author rdl 
*/
int read_serial(int serfd)
{
    int finished = 0;
    unsigned char ch =0;
    int numread = 0;

    do 
    {
        finished++;
        int r = read(serfd, &ch, 1);
        if (r < 0) {
            if (errno == EWOULDBLOCK) {
                usleep(5000);
                continue;
            }
            printf("serial read returned %d\n", r);
            return r;
        }
        inbuffer[numread] = ch;
        if (ch == '\r' && inbuffer[numread - 1] == '\n') finished = 256;
        if (r == 1) {
            numread += r;
            finished--;
        }
    }
    while ( numread < 256 && finished < 256);
    
    if (debug)
        dumphex(inbuffer, numread, 0);

    return numread;
}

/**
\brief  Close the serial port
\author rdl 
*/
void close_serial(int serfd)
{
    if (serfd < 0) return;
    close(serfd);
}

/**
\brief  Setup the inverter for comms.
\author rdl 
*/
int init_inverter(int serfd)
{
    int len = 0;
    char ctl  = 0x30;
    char func = 0x44;
    int cmdlength = create_command(ctl, func, NULL, 0);
    
    if (write_serial(serfd, cmdlength) != cmdlength) {
	return -1;
    }
    // wait before sending next command
    sleep(1);

    func = 0x40;
    cmdlength = create_command(ctl, func, NULL, 0);
    if (write_serial(serfd, cmdlength) == cmdlength) {
        // get response
        len = read_serial(serfd);
        if (len < headerlen) return -1;
    }
    // wait before sending next command
    sleep( 1 );

    func = 0x41;
    // get the serial number from the response
    unsigned char serno[256];
    memcpy(serno, &inbuffer[headerlen], inbuffer[6]);
    // set the device id
    serno[inbuffer[6]] = 1;
    // now register the inverter as device id 1
    cmdlength = create_command(ctl, func, serno, inbuffer[6] + 1);
    if (write_serial(serfd, cmdlength) == cmdlength) {
        // get response
        len = read_serial(serfd);
        if (len < headerlen) return -1;
    }

    return len;
}

/**
\brief  Read the inverter data.
\author rdl 
*/
int read_inverter(int serfd)
{
    int len = 0;
    char ctl  = 0x31;
    char func = 0x42;
    destaddr = 1;
    int cmdlength = create_command(ctl, func, NULL, 0);
    
    if (write_serial(serfd, cmdlength) == cmdlength) {
        // get response
        len = read_serial(serfd);
        if (len < headerlen) return -1;
    }
    return len;
}

/**
\brief  Writes the inverter data
\author rdl 
*/
int output_inverter(int csv_flag, int len)
{
    if (len < headerlen + 20) return -1;
    unsigned char *buf = &inbuffer[headerlen];

    float temp   = ((buf[0] << 8)  + buf[1]) / 10.0;
    float todayE = ((buf[2] << 8)  + buf[3]) / 100.0;
    float VDC    = ((buf[4] << 8)  + buf[5]) / 10.0;
    float I      = ((buf[6] << 8)  + buf[7]) / 10.0;
    float VAC    = ((buf[8] << 8)  + buf[9]) / 10.0;
    float freq   = ((buf[10] << 8) + buf[11]) / 100.0;
    float currE  = ((buf[12] << 8) + buf[13]);
    float unk1   = ((buf[14] << 8) + buf[15]);
    float unk2   = ((buf[16] << 8) + buf[17]);
    float totE   = ((buf[18] << 8) + buf[19]) / 10.0;

    char str[512];
    bzero(str, 512);
    snprintf(str, 512, "temp:%f TodayE:%f VDC:%f I:%f VAC:%f Freq:%f CurrE:%f unk1:%f unk2:%f TotE:%f\n",
        temp, todayE, VDC, I, VAC, freq, currE, unk1, unk2, totE );

    log_output(LOG_WARNING, str);

    return 0;
}

/**
\brief	bit of a test
*/
int test_inverter()
{
	printf("test\n");
	debug = 1;
	char *tmp = "A5A5010131BD2A0174099006050003097D13890000FFFF00005E1A000005D8000100000000000000000000000000000000F80A0A0D";
	int msglen = strlen(tmp) / 2;
	int idx;
	unsigned char val;
	for(idx = 0; idx < msglen; idx++) {
        sscanf(tmp + idx*2, "%2hhx", &val);
        inbuffer[idx] = val;
    }
    output_inverter(0,msglen);
	printf("end test\n");
    return 0;
}
/**
\brief  main program entry point.
\author rdl 
*/
int main(int argc, char *argv[])
{
    int csv_flag = 0, opt, retval = 0;

    while ((opt = getopt(argc, argv, "dfp:l:")) != -1) {
        switch (opt)
        {
        case 'd':
        	debug = 1;
        	break;
        case 'f':
            csv_flag = 1;
            break;
        case 'p':
            serial_device = optarg;
            break;
        case 'l':
            log_path = optarg;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-p /dev/ttyS0] [-f] [-l /var/log/solarmonj/]\n",argv[0]);
            fprintf(stderr, "[-p /dev/ttyS0]          The serial port that the JFY is connected to\n");
            fprintf(stderr, "[-l /var/log/solarmonj/] Path to log\n");
            fprintf(stderr, "[-f]                     Whether to log to the csv file\n");
            exit(EXIT_FAILURE);
        }
    }

    int serfd = open_serial(serial_device);
    if (serfd < 0) {
    	log_output(LOG_ERR, "Failed to open serial port.");
    	retval = 1;
    	goto exit;
    }
    if (init_inverter(serfd) <  0) {
    	log_output(LOG_ERR, "Failed to register inverter.");
    	retval = 2;
    	goto exitclose;
    }
    int len = read_inverter(serfd);
	if (len < 0) {
    	log_output(LOG_ERR, "Failed to read inverter data.");
    	retval = 3;
    	goto exitclose;
    }

	output_inverter(csv_flag, len);

exitclose:
    close(serfd);
exit:
    serfd = -1;
    return retval;    
}

