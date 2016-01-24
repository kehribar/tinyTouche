/*-----------------------------------------------------------------------------
/
/
/------------------------------------------------------------------------------
/ ihsan Kehribar - 2016
/----------------------------------------------------------------------------*/
#include <svm.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "serial_lib.h"
#include "globalDefinitions.h"

#define NOSAMPS 150

int x;
double r;
int port;
int last_key;
int noLabels;
int noDecVals;
double *decVals;
int key_cnt = 0;
int samp[NOSAMPS];
int rmin[NOSAMPS];
int rmax[NOSAMPS];
uint8_t buf[4096];
FILE* training_file;
int training_keyCnt;
uint16_t points[150];
int training_idle = 1;
int training_mode = 0;
struct svm_model *model;
int training_gestureCount;
int training_sampleInd = 0;
int training_gestureInd = 0;
int training_samplePerGesture;
int classification_enable = 0;
struct svm_node features[NOSAMPS+1];

int sweepCap_flush(int fd);
int sweepCap_getData(int fd, uint16_t* ptr);
int classifyInput_init(const char* modelfile);
void platform_key(unsigned char key, int x, int y);
int loadRangeFile(char* rangeFile, int* rmin, int* rmax);

/* This function is automatically called when the openGL engine is idle */
uint8_t fillScreenBuffer(double* screenBufferPointer,uint16_t size,uint16_t topValue)
{      
  double tmpVal;    
  int screenIndex = 0;      

  if(training_mode)
  { 
    if(training_idle == 1)
    {
      printf(">> Get ready for the class #%d\n",training_gestureInd);
      printf(">> Press x when you are ready ... \n");                  
      training_idle = 2;
      training_keyCnt = key_cnt;
      return 0;
    }
    else if(training_idle == 2)
    {
      if((training_keyCnt != key_cnt) && (last_key == 'x'))
      {
        printf(">> Data collecting ...\n");
        sweepCap_flush(port);
        training_idle = 0;
      }
      return 0;
    }
  }

  if(sweepCap_getData(port,points) == 0)
  {
    return 0;
  }

  if(training_mode)
  {     
    fprintf(training_file, "%d ", training_gestureInd + 1);    

    for(int i=0; i < 150; i++)
    {
       fprintf(training_file, " %d:%d", i+1, points[i]);
       if(i == 149)
       {
        fprintf(training_file, "\n");
       }
    }

    printf(">> Class #%d - Sample #%d\n",training_gestureInd,training_sampleInd);
    training_sampleInd++;
    if(training_sampleInd == training_samplePerGesture)
    {
      training_idle = 1;
      training_sampleInd = 0;

      printf(">> Data collecting end for the class #%d\n", training_gestureInd);
      training_gestureInd++;

      if(training_gestureInd == training_gestureCount)
      {        
        training_mode = 0;
        fclose(training_file);
        printf(">> Data collecting end completely!\n");       
        printf(">> Training start ...\n"); 

        /* Run the easy.py script from libsvm */
        system("./svm_files/easy.py ./svm_files/training_set.svm");

        printf(">> Training done.\n"); 
        printf(">> Press enter to exit the program ...\n");
        getchar();
        exit(0);
      }
    }
  }
  
  for(int i = 0; i < 150; i++)
  {    
    // scale
    tmpVal = (double)points[i] * (double)HEIGHT / (double)1024.0;

    screenBufferPointer[screenIndex++] = tmpVal;    
    screenBufferPointer[screenIndex++] = tmpVal;    
    screenBufferPointer[screenIndex++] = tmpVal;    
    screenBufferPointer[screenIndex++] = tmpVal;    
  }  

  if(classification_enable == 1)
  {
    for(x=0; x<NOSAMPS; x++) 
    {
      features[x].index = x;
      
      // rescale to [-1, 1]
      r = (points[x]-rmin[x+1]);
      r = r/(rmax[x+1]-rmin[x+1]);
      r = (r*2)-1;

      features[x+1].value = r;
    }

    features[x].index = -1;
    r = svm_predict_values(model,features,decVals);
    printf(">> Gesture number: %d\n", (int)r);
  }

  return 1; 
}

/* Return 0 for successful hardware/software platform initialization */
int platform_init(int argc, char** argv)
{  
  char temp[1024];

  if (argc < 2) 
  {
    printf("Example: serial_listen [port_path]\n");
    return -1;
  }

  if(argc == 3)
  {
    if(strcmp(argv[2],"train") == 0)
    {
      printf(">> Training mode enabled\n");

      printf(">> How many classes?\n");
      printf(">> ");  
      fgets(temp,sizeof(temp),stdin);
      sscanf(temp,"%d",&training_gestureCount);
      if(training_gestureCount>0)  
      {
        printf(">> You wanted %d classes\n",training_gestureCount);
      }
      else
      {
        printf(">> error... \n");
        return 0;
      }
      
      printf(">> How many sample for each class?\n");
      printf(">> ");
      fgets(temp,sizeof(temp),stdin);
      sscanf(temp,"%d",&training_samplePerGesture);
      if(training_samplePerGesture>0) 
      {
        printf(">> You wanted %d samples for each gesture class\n",training_samplePerGesture);
      }
      else
      {
        printf(">> error... \n");
        return 0;
      }

      training_file = fopen("./svm_files/training_set.svm","w+");
      if(training_file == NULL)
      {
        printf(">> File err!\n");        
        return 0;
      }

      training_mode = 1;
    }
  }   

  // Open the serial port
  port = serialport_init(argv[1],115200,'N');
  if (port < 0) 
  {
    printf("Unable to open %s\n", argv[1]);
    return -1;
  }  

  if(classifyInput_init("./svm_files/training_set.svm") == 0)
  {
    classification_enable = 1;
    printf(">> Classification file is OK!\n");
  }
  else
  {
    classification_enable = 0;
    printf(">> Classification file is non-existent!\n");
  }

  sweepCap_flush(port);

  printf(">> Ok!\n");  

  return 0;
}

int loadRangeFile(char* rangeFile, int* rmin, int* rmax)
{
  FILE *f;
  int i, min, max;
  char buff[1024];
  
  f = fopen(rangeFile, "r");
  
  if(f == NULL) 
  {
    perror("loading rangefile");
    return -1;
  }
  
  fgets(buff, 1024, f); // first line is bogus
  fgets(buff, 1024, f); // second line is bogus
  
  while(fscanf(f, "%d %d %d\n", &i, &min, &max) > 0) 
  {
    if(i > 0)
    {
      rmin[i] = min;
      rmax[i] = max;
    }
  }
  
  fclose(f);
  return 0;
}

int classifyInput_init(const char* modelfile) 
{
  char buff[1024];

  strcpy(buff, modelfile);
  strcat(buff, ".range");
  if(loadRangeFile(buff, rmin, rmax) != 0)
  {
    return -1;
  }

  strcpy(buff, modelfile);
  strcat(buff, ".model");

  model = svm_load_model(buff);  
  if(model == NULL) 
  {
    printf("Couldn't load model!\n");
    return -1;
  }

  noLabels = svm_get_nr_class(model);
  noDecVals = noLabels * ((noLabels-1)/2);
  decVals = (double*)malloc(sizeof(double) * noDecVals);

  return 0;
}

int sweepCap_getData(int fd, uint16_t* ptr)
{
  int syncCnt = 0;  

  while(syncCnt != 4)
  {    
    if(readRawBytes(port, (char*)buf, 1, 100) != 0)
    {     
      return 0;
    }

    if((syncCnt == 1) && (buf[0] == 0xAD))
    {      
      syncCnt = 2;      
    }
    else if((syncCnt == 2) && (buf[0] == 0xBE))
    {
      syncCnt = 3;      
    }
    else if((syncCnt == 3) && (buf[0] == 0xEF))      
    {
      syncCnt = 4;      
    }
    else
    {
      if(buf[0] == 0xDE)
      {
        syncCnt = 1;
        
      }
      else
      {
        syncCnt = 0;  
      }      
    }       
  }    

  if(readRawBytes(port, (char*)buf, (150 * 2), 100) != 0)
  {    
    return 0;
  }  

  uint8_t checksum = 0;

  // calculate the checksum
  for(int i = 0; i < 150; i++)
  {
    points[i] = (uint16_t)buf[(2*i)] + (uint16_t)((uint16_t)buf[(2*i)+1] << 8);
    
    checksum += buf[(2*i)];
    checksum += buf[(2*i)+1];
  }  

  /* get the checksum */
  if(readRawBytes(port, (char*)buf, 1, 100) != 0)
  {     
    return 0;
  }

  if(buf[0] != checksum)
  {
    return 0;
  }

  return 1;
}

int sweepCap_flush(int fd)
{
  int rv; 
  uint8_t junkBuffer[4];  
  do
  {    
    rv = read(fd, junkBuffer, 4);
  }
  while(rv == 4);

  return 0;
}

void platform_key(unsigned char key, int x, int y)
{
  key_cnt++;
  last_key = key;
}
