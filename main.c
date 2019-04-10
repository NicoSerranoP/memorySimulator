#include<stdio.h>
#include<stdlib.h>

int main(int argc, char const *argv[]) {

  //Command line arguments variables
  FILE *fp;
  int version;
  int debug;

  //Check if the debug optional argument was set
  if(argc==4){
    fp = fopen(argv[1],"r"); //Command line argument parameters file
    version = atoi(argv[2]); //Command line argument (1 or 2)
    debug = atoi(argv[3]);  // Command line argument optional
  }
  else{
    fp = fopen(argv[1],"r");
    version = atoi(argv[2]); //Command line argument (1 or 2)
  }

  //Parameters variables
  int pageSize = 0; // (512, 1024, 2048)
  int memFrames = 0; // (16, 32, 64)
  int quantumRR = 0; //number of process' instructions that are executed before selecting the process
  int numProcesses = 0; //number of processes
  char comment[20]; //auxiliar variable to save definition

  //Extracting parameters from command line passed file
  fscanf(fp,"%d %s",&pageSize,comment);
  fscanf(fp,"%d %s",&memFrames,comment);
  fscanf(fp,"%d %s",&quantumRR,comment);
  fscanf(fp,"%d %s",&numProcesses,comment);
  printf("Size: %d\n",pageSize);
  printf("Frames: %d\n",memFrames);
  printf("Quantum: %d\n",quantumRR);
  printf("Processes: %d\n",numProcesses);

  //Getting the processes' instructions file names
  char pNames[numProcesses][13];
  for(int i=0; i<numProcesses; i++){
    fscanf(fp,"%s ",pNames[i]);
    printf("pNames[%d]: %s \n",i,pNames[i]);
  }

  //Opening the processes's instructions file names
  int pID=0,aOperation=0,aValue=0;
  char action;
  FILE *pFiles[numProcesses];
  for(int i=0; i< numProcesses; i++){
    pFiles[i] = fopen(pNames[i],"r");
    fscanf(pFiles[i],"%d %d %d %c",&pID,&aOperation,&aValue,&action);
    printf("PID:%d Operation:%d Value:%d Action:%c\n",pID,aOperation,aValue,action);
    fclose(pFiles[i]);
  }

  return 0;
}
