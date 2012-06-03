/*
	ihsan Kehribar
	June 2012

	Basic SVM classifier + OpenGL visualiser 	for tinyTouche project.
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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <cstring> 
#include <string.h>
#include <GL/glut.h>
#include <libsvm/svm.h>

#define BAUDRATE B115200

#define NOSAMPS 128

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


FILE *f;


unsigned int val;
int detection;
int com;
int ax,ay,az;
int mx,my,mz;
char* fileName="train.svm"	;

int x_old,x_new,y_old,y_new,z_old,z_new;

const int buffSize = NOSAMPS ;

const int step=1024/buffSize;
unsigned int dataBuffer[buffSize];

const float A = 1024-step;  // width 
const float B = 512;  // height

void myinit(void)
{
  glClearColor(0.7, 0.7, 0.7, 0.0); // gray background
  glMatrixMode(GL_PROJECTION);      
  glLoadIdentity();                 
  gluOrtho2D( 0, A, 0, B); // defining the corner points of the window
  glMatrixMode(GL_MODELVIEW);       
}

void display( void )
{
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);       
	glLoadIdentity();         

	int i=0;
	int xAxis=0;
	char* myNumber;

	glBegin(GL_LINES);     
		for(i=0;i<(buffSize-1);i++)
		{
			glColor3f ( 1.0, 0.0, 0.0);       
			glVertex2f( xAxis, dataBuffer[i] /2 );         
			glVertex2f( xAxis+step, dataBuffer[i+1] /2);            
			xAxis += step ;  
		}
	glEnd();

	sprintf(myNumber,"%d",detection);
	
	glColor3f ( 0.0, 0.0, 0.0);      

 	glTranslatef(650,100,0);
	glScaled(2,2,1);
	glutStrokeCharacter(GLUT_STROKE_ROMAN,myNumber[0]);
	
	glFlush();                        
}


/*	In order to avoid the sprintf at the microcontroller side,
	data is sent in raw byte format. To make sure that we read
	the line properly, check that the first and the last 
	meaningful byte of the read buffer is equal to '#' or not.
	If not, skip this reading, continue to use the old one. */
void getSamples(unsigned int* inBuffer)
{
	int n;
	int asdas = 0;
	int qq;
	unsigned char buff[4096];
	unsigned char tem;
	
	fgets((char *)buff,1024,f);
	fflush(f);

	if(buff[0]=='#' && buff[(2*buffSize)+1]=='#')
	{
		for(n=0;n<buffSize;n++)
		{
			val = buff[(2*n)+1];			
			val += buff[(2*n)+2]<<8;
			inBuffer[asdas]=val;
			asdas++;
			//printf("%d,",val);
		}
		// printf("\n");
	}
	else
		printf("err..");

	
}

void loadRangeFile(char *rangeFile, int rmin[], int rmax[]) {
	FILE *f;
	int i, min, max;
	char buff[1024];
	f=fopen(rangeFile, "r");
	if (f==NULL) {
		perror("loading rangefile");
		exit(1);
	}
	fgets(buff, 1024, f); //first line is bogus
	fgets(buff, 1024, f); //second line is bogus
	while(fscanf(f, "%d %d %d\n", &i, &min, &max)>0) {
		if (i>0) {
			rmin[i]=min;
			rmax[i]=max;
			//printf("%d %d %d\n", i, rmin[i], rmax[i]);
		}
	}
	fclose(f);
}


struct svm_model *model;
struct svm_node features[NOSAMPS+1];
int samp[NOSAMPS];
int rmin[NOSAMPS];
int rmax[NOSAMPS];
double *decVals;
int x, noLabels, noDecVals;
double r;
void init_classifyInput(char *modelfile) {

	char buff[1024];

	strcpy(buff, modelfile);
	strcat(buff, ".range");
	loadRangeFile(buff, rmin, rmax);

	strcpy(buff, modelfile);
	strcat(buff, ".model");
	model=svm_load_model(buff);
	if (model==NULL) {
		printf("Couldn't load model!\n");
		exit(0);
	}
	noLabels=svm_get_nr_class(model);
	noDecVals=noLabels*(noLabels-1)/2;
	decVals=(double*)malloc(sizeof(double)*noDecVals);
}


void Idle() {	 

	getSamples(dataBuffer);

	for (x=0; x<NOSAMPS; x++) {
		features[x].index=x;
		//rescale to [-1, 1]
		r=(dataBuffer[x]-rmin[x+1]);
		r=r/(rmax[x+1]-rmin[x+1]);
		r=(r*2)-1;
		features[x+1].value=r;
//			printf("%f ", features[x].value);
	}
//		printf("\n");
	features[x].index=-1;
	r=svm_predict_values(model,features,decVals);
	detection = (int) r;
	printf("gesture number: %f\n", r);

	glutPostRedisplay();    // Post a paint request to activate display()
}


/* Compile with g++ main.cpp -lglut -lGLU -lsvm */
int main(int argc, char* argv[])
{

	char buff[1024];
	int n;

	com=serialOpen("/dev/ttyUSB0");
	f=fdopen(com, "r");

	int A_,B_; // For window init.

	A_= (int) A;
	B_= (int) B;
      
	for(int i=0;i<buffSize;i++)
		dataBuffer[i]=0;

	glutInit(&argc,argv);
	glutInitWindowSize( A_ , B_ );     
	
	init_classifyInput(fileName); 

	glutInitDisplayMode( GLUT_RGB | GLUT_SINGLE);
	glutCreateWindow("Frequency sweep cap. measure!"); 
	glutDisplayFunc(display);
	glutIdleFunc(Idle);
	glutIgnoreKeyRepeat(1);  
	myinit();                        

	glutMainLoop();                         

	system("PAUSE");
	return 0;
}

