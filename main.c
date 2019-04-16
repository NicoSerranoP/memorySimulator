//./a.out param.txt 1
#include<stdio.h>
#include<string.h>
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
  fclose(fp);

  //Creating the pageTable to be used (one row is one page loaded at a frame)
  int pTable[memFrames][5];
  for (int i=0; i<memFrames; i++)
    for (int j=0; j<5; j++)
      pTable[i][j] = 0;
  //Defining each column as the attributes of each page
  int pID=0, page=1, ref=2, dirty=3, counter=4; //clock
  //Defining variables for the output result
  int pageFaults=0, diskRef=0;

  //Defining temporal variable to pick up attributes from txt file
  int pIDs=0,aOperation=0,aValue=0;
  char action;
  //cont is a counter for the quantum process
  //pro is representing the process index
  //sww is a switch to stop the while when all processes finishes
  //swt is a switch variable to know if a page has fault and been changed
  int cont=0, pro=0, sww=1, swt=1;
  //instContrl is a control variable to know if all instructions have been readed from the txt file
  int instContrl;
  //Pointer array to open all txt files (one per process)
  FILE *pFiles[numProcesses];
  int proArray[numProcesses];
  //Go throught all processes and open their txt files
  for(int i=0; i<numProcesses; i++){
    pFiles[i] = fopen(pNames[i],"r");
    //if proArray[i] is zero then it means process i has not been finished
    proArray[i] = 0;
  }

  //sww is a switch to stop the while when all processes finishes
  while(sww){
    //if the quantum has been reach, then start the next process
    if(cont==quantumRR){
      cont=0;
      do{
        //if the process index is inside the interval, pass to the next index
        //otherwise start over with index equals zero
        if (pro<numProcesses-1) pro++;
        else pro=0;
        //This switch variable is turned off
        sww=0;
        for(int i=0; i<numProcesses; i++){
          //printf("proArray[%d]:%d\n",i,proArray[i]);
          //if any process has not been finished (0), then turn sww on
          if(proArray[i]==0){
            sww = 1;
          }
        }
      } while(proArray[pro]==1 && sww);
      //if the process at current index has finished, then keep the loop
      //if the sww for all processes to be finished is on, then keep the loop
    }

    //get the instructions variables
    instContrl=fscanf(pFiles[pro],"%d %d %d %c",&pIDs,&aOperation,&aValue,&action);
    //if the number of variables readed is four (PID, Operation, Value, Action) then continue
    if(instContrl==4){
      //swt is a switch variable to know if a page has fault and been changed
      swt = 1;
      //check out all frames
      for(int i=0; i<memFrames; i++){
        //if the page number (aOperation/pageSize) of process pIDS is loaded in the frame
        if(pTable[i][pID]==pIDs && pTable[i][page]==(aOperation/pageSize)){
          //Keep going normally. The Operation address is read only and it is reading from the page
          //Set up the reference bit for this page
          pTable[i][ref] = 1;
          //turn off the switch swt
          swt = 0;
          //printf("Page %d of pID:%d was in the frame\n",(aOperation/pageSize),pIDs);
        }
      }
      //VERSION ONE (second argument is 1)
      if(version==1){
        //if the switch is on, then there was a pageFault
        if(swt){
          //count up a page fault
          pageFaults++;
          //check if there is any page which has not been referenced and it is not dirty
          for(int i=0; i<memFrames; i++){
            if(pTable[i][ref]==0 && pTable[i][dirty] ==0){
              //if you found a page, then load all the new information
              pTable[i][pID] = pIDs;
              pTable[i][page] = aOperation/pageSize;
              pTable[i][ref] = 1;
              pTable[i][dirty] = 0;
              pTable[i][counter] =1; //This variable is not useful in version 1
              //turn off the switch swt
              swt = 0;
              //count up a disk references because of the loading process of the page
              diskRef++;
              //printf("(0,0) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
            }
          }
          //if the switch is still on then search with other criteria
          if(swt){
            //check if there is any page which has not been referenced but it is dirty
            for(int i=0; i<memFrames; i++){
              if(pTable[i][ref]==0 && pTable[i][dirty]==1){
                //if you found a page, then load all the new information
                pTable[i][pID] = pIDs;
                pTable[i][page] = aOperation/pageSize;
                pTable[i][ref] = 1;
                pTable[i][dirty] = 0;
                pTable[i][counter] =1; //This variable is not useful in version 1
                //turn off the switch swt
                swt = 0;
                //count up twice a disk reference because of the saving process and the loading process of the pages
                diskRef = diskRef + 2;
                //printf("(0,1) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
              }
            }
          }
          //if the switch is still on then search with other criteria
          if(swt){
            //check if there is any page which has been referenced but it is not dirty
            for(int i=0; i<memFrames; i++){
              if(pTable[i][ref]==1 && pTable[i][dirty]==0){
                //if you found a page, then load all the new information
                pTable[i][pID] = pIDs;
                pTable[i][page] = aOperation/pageSize;
                pTable[i][ref] = 1;
                pTable[i][dirty] = 0;
                pTable[i][counter] =1; //This variable is not useful in version 1
                //turn off the switch swt
                swt = 0;
                //count up a disk references because of the loading process of the page
                diskRef++;
                //printf("(1,0) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
              }
            }
          }
          //if the switch is still on then search with other criteria
          if(swt){
            //the only option is a page which has been referenced and it is dirty
            pTable[0][pID] = pIDs;
            pTable[0][page] = aOperation/pageSize;
            pTable[0][ref] = 1;
            pTable[0][dirty] = 0;
            pTable[0][counter] =1; //This variable is not useful in version 1
            //count up twice a disk reference because of the saving process and the loading process of the pages
            diskRef = diskRef + 2;
            //printf("(1,1) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
          }
        }
      }

      //VERSION TWO (second argument is 2)----------------------------------------------------------------------------------------
      //NOT READY YET
      if(version==2){
        if(swt){
          for(int i=0; i<memFrames; i++){
            if(pTable[i][ref]==0 && pTable[i][dirty] ==0){
              pTable[i][pID] = pIDs;
              pTable[i][page] = aOperation/pageSize;
              pTable[i][ref] = 1;
              pTable[i][dirty] = 0;
              pTable[i][counter] =1; //Please help!
              swt = 0;
              //printf("(0,0) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
            }
          }
          if(swt){
            for(int i=0; i<memFrames; i++){
              if(pTable[i][ref]==0 && pTable[i][dirty]==1){
                pTable[i][pID] = pIDs;
                pTable[i][page] = aOperation/pageSize;
                pTable[i][ref] = 1;
                pTable[i][dirty] = 0;
                pTable[i][counter] =1; //Please help!
                swt = 0;
                //printf("(0,1) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
              }
            }
          }
          if(swt){
            for(int i=0; i<memFrames; i++){
              if(pTable[i][ref]==1 && pTable[i][dirty]==0){
                pTable[i][pID] = pIDs;
                pTable[i][page] = aOperation/pageSize;
                pTable[i][ref] = 1;
                pTable[i][dirty] = 0;
                pTable[i][counter] =1; //Please help!
                swt = 0;
                //printf("(1,0) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
              }
            }
          }
          if(swt){
            pTable[0][pID] = pIDs;
            pTable[0][page] = aOperation/pageSize;
            pTable[0][ref] = 1;
            pTable[0][dirty] = 0;
            pTable[0][counter] =1; //Please help!
            //printf("(1,1) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
          }
        }
      }
      //VERSION TWO (second argument is 2)----------------------------------------------------------------------------------------

      //The exact same process but for Value address now
      //swt is a switch variable to know if a page has fault and been changed
      swt = 1;
      for(int i=0; i<memFrames; i++){
        //if the page number (aValue/pageSize) of process pIDS is loaded in the frame
        if(pTable[i][pID]==pIDs && pTable[i][page]==(aValue/pageSize)){
          //Set up the reference bit for this page
          pTable[i][ref] = 1;
          //if the action was to write then set up the dirty bit for this page
          if(action=='W') pTable[i][dirty] = 1;
          //turn off the switch swt
          swt = 0;
          //printf("Page %d of pID:%d was in the frame\n",(aValue/pageSize),pIDs);
        }
      }
      //VERSION ONE (second argument is 1)
      if(version==1){
        //if the switch is on, then there was a pageFault
        if(swt){
          //count up a page fault
          pageFaults++;
          //check if there is any page which has not been referenced and it is not dirty
          for(int i=0; i<memFrames; i++){
            if(pTable[i][ref]==0 && pTable[i][dirty] ==0){
              //if you found a page, then load all the new information
              pTable[i][pID] = pIDs;
              pTable[i][page] = aValue/pageSize;
              pTable[i][ref] = 1;
              //if the action was to write then set up the dirty bit for this page
              if(action=='W') pTable[i][dirty] = 1;
              else pTable[i][dirty] = 0;
              pTable[i][counter] =1; //This variable is not useful in version 1
              //turn off the switch swt
              swt = 0;
              //count up a disk references because of the loading process of the page
              diskRef++;
              //printf("(0,0) Page %d of pID:%d was loaded\n",(aValue/pageSize),pIDs);
            }
          }
          //if the switch is still on then search with other criteria
          if(swt){
            //check if there is any page which has not been referenced but it is dirty
            for(int i=0; i<memFrames; i++){
              if(pTable[i][ref]==0 && pTable[i][dirty]==1){
                //if you found a page, then load all the new information
                pTable[i][pID] = pIDs;
                pTable[i][page] = aValue/pageSize;
                pTable[i][ref] = 1;
                //if the action was to write then set up the dirty bit for this page
                if(action=='W') pTable[i][dirty] = 1;
                else pTable[i][dirty] = 0;
                pTable[i][counter] =1; //This variable is not useful in version 1
                //turn off the switch swt
                swt = 0;
                //count up twice a disk reference because of the saving process and the loading process of the pages
                diskRef = diskRef + 2;
                //printf("(0,1) Page %d of pID:%d was loaded\n",(aValue/pageSize),pIDs);
              }
            }
          }
          //if the switch is still on then search with other criteria
          if(swt){
            //check if there is any page which has been referenced but it is not dirty
            for(int i=0; i<memFrames; i++){
              if(pTable[i][ref]==1 && pTable[i][dirty]==0){
                //if you found a page, then load all the new information
                pTable[i][pID] = pIDs;
                pTable[i][page] = aValue/pageSize;
                pTable[i][ref] = 1;
                //if the action was to write then set up the dirty bit for this page
                if(action=='W') pTable[i][dirty] = 1;
                else pTable[i][dirty] = 0;
                pTable[i][counter] =1; //This variable is not useful in version 1
                //turn off the switch swt
                swt = 0;
                //count up a disk references because of the loading process of the page
                diskRef++;
                //printf("(1,0) Page %d of pID:%d was loaded\n",(aValue/pageSize),pIDs);
              }
            }
          }
          //if the switch is still on then search with other criteria
          if(swt){
            //the only option is a page which has been referenced and it is dirty
            pTable[0][pID] = pIDs;
            pTable[0][page] = aValue/pageSize;
            pTable[0][ref] = 1;
            //if the action was to write then set up the dirty bit for this page
            if(action=='W') pTable[0][dirty] = 1;
            else pTable[0][dirty] = 0;
            pTable[0][counter] =1; //This variable is not useful in version 1
            //count up twice a disk reference because of the saving process and the loading process of the pages
            diskRef = diskRef + 2;
            //printf("(1,1) Page %d of pID:%d was loaded\n",(aValue/pageSize),pIDs);
          }
        }
      }

      //VERSION TWO (second argument is 2)----------------------------------------------------------------------------------------
      if(version==2){
        if(swt){
          for(int i=0; i<memFrames; i++){
            if(pTable[i][ref]==0 && pTable[i][dirty] ==0){
              pTable[i][pID] = pIDs;
              pTable[i][page] = aOperation/pageSize;
              pTable[i][ref] = 1;
              pTable[i][dirty] = 0;
              pTable[i][counter] =1; //Please help!
              swt = 0;
              printf("(0,0) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
            }
          }
          if(swt){
            for(int i=0; i<memFrames; i++){
              if(pTable[i][ref]==0 && pTable[i][dirty]==1){
                pTable[i][pID] = pIDs;
                pTable[i][page] = aOperation/pageSize;
                pTable[i][ref] = 1;
                pTable[i][dirty] = 0;
                pTable[i][counter] =1; //Please help!
                swt = 0;
                printf("(0,1) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
              }
            }
          }
          if(swt){
            for(int i=0; i<memFrames; i++){
              if(pTable[i][ref]==1 && pTable[i][dirty]==0){
                pTable[i][pID] = pIDs;
                pTable[i][page] = aOperation/pageSize;
                pTable[i][ref] = 1;
                pTable[i][dirty] = 0;
                pTable[i][counter] =1; //Please help!
                swt = 0;
                printf("(1,0) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
              }
            }
          }
          if(swt){
            pTable[0][pID] = pIDs;
            pTable[0][page] = aOperation/pageSize;
            pTable[0][ref] = 1;
            pTable[0][dirty] = 0;
            pTable[0][counter] =1; //Please help!
            printf("(1,1) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
          }
        }
      }
      //VERSION TWO (second argument is 2)----------------------------------------------------------------------------------------

      //printf("PID:%d Operation:%d Value:%d Action:%c\n",pIDs,aOperation,aValue,action);
      //printf("cont:%d instContrl:%d\n",cont,instContrl);
    }
    else{
      //if the number of variables readed is not 4, set that process as done (1)
      proArray[pro] = 1;
      //printf("Process %d is over\n",pro+1);
    }
    //increment the quantum counter
    cont++;
  }

  printf("Page Faults: %d\n",pageFaults);
  printf("Disk References: %d\n",diskRef);
  return 0;
}
