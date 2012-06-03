/*
	ihsan Kehribar
	June 2012

	Basic SVM training file generator for tinyTouche project
	https://github.com/kehribar/tinyTouche

	This project is based on Disney Touché 
	http://www.disneyresearch.com/research/projects/hci_touche_drp.htm

	Usage	of the libSVM and the most of the practical information and
	some code snippets are taken from the Sprite_tm's engarde project.
	http://spritesmods.com/?art=engarde
		
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
			 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
					 
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>

#define BAUDRATE B115200

int serialOpen(char *port) {
	struct termios oldtio,newtio;
	int com;
	com=open(port, O_RDWR | O_NOCTTY);
	if (com<0) {
		perror("Opening comport"); 
		exit(1);
	}
	tcgetattr(com,&oldtio); // save current port settings
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag =  (CS8 | CREAD | CLOCAL);
	newtio.c_lflag = 0;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	cfsetospeed(&newtio,BAUDRATE);
	newtio.c_cc[VTIME]    = 0;   // inter-character timer unused 
	newtio.c_cc[VMIN]     = 1;   // blocking read until 1 chars received 
	tcflush(com,TCIFLUSH);
	tcsetattr(com,TCSANOW,&newtio);
	return com;
}

int main(int argc, char* argv[])
{
	int com;
	int asdas;
	int buffSize=128;
	unsigned char buff[1024];
	unsigned int inBuffer[buffSize];
	unsigned int val;
	
	FILE *fp;
	FILE *f;
	
	com=serialOpen("/dev/ttyUSB0");
	f=fdopen(com, "r");
	
	char *name = "./train.svm";
	fp = fopen(name, "w");
	char newLine[4096];
	char temp[1024];
	int index=0;
	int maxClass;
	int maxSample;
	int i,n,q;
	int retry=0;
	
	printf(">> tinyTouche trainer \n");
	
	printf(">> How many classes?\n");
	printf(">> ");
	gets(temp);
	sscanf(temp,"%d",&maxClass);
	if(maxClass>0)	
		printf(">> You wanted %d classes\n",maxClass);
	else
	{
		printf(">> error... \n");
		return 0;
	}
	
	printf(">> How many sample for each class?\n");
	printf(">> ");
	gets(temp);
	sscanf(temp,"%d",&maxSample);
	if(maxSample>0)	
		printf(">> You wanted %d samples\n",maxSample);
	else
	{
		printf(">> error... \n");
		return 0;
	}
	
	
	for(i=0;i<maxClass;i++)
	{
		
		printf(">> Get ready for the class #%d\n",i+1);
		printf(">> Press enter when you are ready ... ");
		getchar();
		printf(">> Data collecting ...\n");
	
		for(q=0;q<maxSample;q++)
		{
			
			if(retry) q=q-1;
		
			fgets((char *)buff,1024,f);
				
			/*	In order to avoid the sprintf at the microcontroller side,
			data is sent in raw byte format. To make sure that we read
			the line properly, check that the first and the last 
			meaningful byte of the read buffer is equal to '#' or not.
			If not, skip this reading */
			if(buff[0]=='#' && buff[(2*buffSize)+1]=='#')
			{
				asdas=0;
				retry=0;

				for(n=0;n<buffSize;n++)
				{
					val = buff[(2*n)+1];			
					val += buff[(2*n)+2]<<8;
					inBuffer[asdas] = val ;
					asdas++;
				}

				fprintf(fp,"%d",i+1);
			
				for(n=0;n<buffSize-1;n++)
				{
					fprintf(fp," %d:%d",n+1,inBuffer[n]);
				}
			
				fprintf(fp," %d:%d\n",buffSize,inBuffer[buffSize-1]);
			}
			else
				retry=1;
		}
		printf(">> Finished class #%d\n",i+1);
	}
	
	printf(">> Finished all!\n");
	printf(">> Running svm-easy ... \n");
	system("svm-easy train.svm");
	
	return 0;
	
}

