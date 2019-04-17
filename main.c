//./a.out param.txt 1
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int main(int argc, char const *argv[]) {

  //Command line arguments variables
  FILE *fp;
  int version;
  int debug=0;

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

  //temporary string to save the file name
  char tempName[13];
  //Pointer array to open all txt files (one per process)
  FILE *pFiles[numProcesses];
  int proArray[numProcesses];
  //Go throught all processes and open their txt files
  for(int i=0; i<numProcesses; i++){
    fscanf(fp,"%s ",tempName);
    pFiles[i] = fopen(tempName,"r");
    //if proArray[i] is zero then it means process i has not been finished
    proArray[i] = 0;
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
  int capturedpID=0,aOperation=0,aValue=0;
  char action;
  //cont is a counter for the quantum process
  //icont is a counter for the number of instructions (after 200, reset the ref bit)
  //pro is representing the process index
  //sww is a switch to stop the while when all processes finishes
  //swt is a switch variable to know if a page has fault and been changed
  //min is used in version 2 to get the minimum value of the counter variable in pTable
  //minposition is used in version 2 to save the minimum value index frame
  int cont=0, icont=0, pro=0, sww=1, swt=1, min=0, minposition=0;
  //instContrl is a control variable to know if all instructions have been readed from the txt file
  int instContrl;


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
    instContrl=fscanf(pFiles[pro],"%d %d %d %c",&capturedpID,&aOperation,&aValue,&action);
    //if the number of variables readed is four (PID, Operation, Value, Action) then continue
    if(instContrl==4){
      //count up the instruction
      icont++;
      //swt is a switch variable to know if a page has fault and been changed
      swt = 1;
      //check out all frames
      for(int i=0; i<memFrames &&swt; i++){
        //printf("pID: %d page: %d \n",pIDs,aOperation/pageSize);
        //if the page number (aOperation/pageSize) of process pIDS is loaded in the frame
        if(pTable[i][pID]==capturedpID && pTable[i][page]==(aOperation/pageSize)){
          //Keep going normally. The Operation address is read only and it is reading from the page
          //Set up the reference bit for this page
          pTable[i][ref] = 1;
          //Set up the counter variable to the current cycle
          pTable[i][counter] = icont;
          //turn off the switch swt
          swt = 0;
          //printf("Page %d of pID:%d was in the frame\n",(aOperation/pageSize),pIDs);
        }
      }

      //VERSION ONE (second argument is 1)
      if(version==1){
        //after 200 instructions, the program sets up the reference bits to 0 of all frames
        if(icont==200){
          icont = 0;
          for(int i=0; i<memFrames; i++)
            pTable[i][ref] = 0;
        }
        //if the switch is on, then there was a pageFault
        if(swt){
          //count up a page fault
          pageFaults++;
          //check if there is any page which has not been referenced and it is not dirty
          for(int i=0; i<memFrames &&swt; i++){
            if(pTable[i][ref]==0 && pTable[i][dirty] ==0){
              //if you found a page, then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aOperation/pageSize),capturedpID,pTable[i][dirty]);
              pTable[i][pID] = capturedpID;
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
          //check if there is any page which has not been referenced but it is dirty
          for(int i=0; i<memFrames &&swt; i++){
            if(pTable[i][ref]==0 && pTable[i][dirty]==1){
              //if you found a page, then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aOperation/pageSize),capturedpID,pTable[i][dirty]);
              pTable[i][pID] = capturedpID;
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
          //if the switch is still on then search with other criteria
          //check if there is any page which has been referenced but it is not dirty
          for(int i=0; i<memFrames &&swt; i++){
            if(pTable[i][ref]==1 && pTable[i][dirty]==0){
              //if you found a page, then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aOperation/pageSize),capturedpID,pTable[i][dirty]);
              pTable[i][pID] = capturedpID;
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

          //if the switch is still on then search with other criteria
          //check if there is any page which has been referenced an it is dirty
          for(int i=0; i<memFrames &&swt; i++){
            if(pTable[i][ref]==1 && pTable[i][dirty]==1){
              //if you found a page, then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aOperation/pageSize),capturedpID,pTable[i][dirty]);
              pTable[i][pID] = capturedpID;
              pTable[i][page] = aOperation/pageSize;
              pTable[i][ref] = 1;
              pTable[i][dirty] = 0;
              pTable[i][counter] =1; //This variable is not useful in version 1
              //turn off the switch swt
              swt = 0;
              //count up twice a disk reference because of the saving process and the loading process of the pages
              diskRef = diskRef + 2;
              //printf("(1,0) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
            }
          }
        }

        //The exact same process but for VALUE ADDRESS now
        //swt is a switch variable to know if a page has fault and been changed
        swt = 1;
        for(int i=0; i<memFrames &&swt; i++){
          //if the page number (aValue/pageSize) of process pIDS is loaded in the frame
          if(pTable[i][pID]==capturedpID && pTable[i][page]==(aValue/pageSize)){
            //Set up the reference bit for this page
            pTable[i][ref] = 1;
            //if the action was to write then set up the dirty bit for this page
            if(action=='W') pTable[i][dirty] = 1;
            //turn off the switch swt
            swt = 0;
            //printf("Page %d of pID:%d was in the frame\n",(aValue/pageSize),pIDs);
          }
        }
        //if the switch is on, then there was a pageFault
        if(swt){
          //count up a page fault
          pageFaults++;
          //check if there is any page which has not been referenced and it is not dirty
          for(int i=0; i<memFrames &&swt; i++){
            if(pTable[i][ref]==0 && pTable[i][dirty] ==0){
              //if you found a page, then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aValue/pageSize),capturedpID,pTable[i][dirty]);
              pTable[i][pID] = capturedpID;
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
          //check if there is any page which has not been referenced but it is dirty
          for(int i=0; i<memFrames &&swt; i++){
            if(pTable[i][ref]==0 && pTable[i][dirty]==1){
              //if you found a page, then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aValue/pageSize),capturedpID,pTable[i][dirty]);
              pTable[i][pID] = capturedpID;
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
          //if the switch is still on then search with other criteria
          //check if there is any page which has been referenced but it is not dirty
          for(int i=0; i<memFrames &&swt; i++){
            if(pTable[i][ref]==1 && pTable[i][dirty]==0){
              //if you found a page, then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aValue/pageSize),capturedpID,pTable[i][dirty]);
              pTable[i][pID] = capturedpID;
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
          //if the switch is still on then search with other criteria
          //check if there is any page which has been referenced an it is dirty
          for(int i=0; i<memFrames &&swt; i++){
            if(pTable[i][ref]==1 && pTable[i][dirty]==1){
              //if you found a page, then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aValue/pageSize),capturedpID,pTable[i][dirty]);
              pTable[i][pID] = capturedpID;
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
              //printf("(1,0) Page %d of pID:%d was loaded\n",(aOperation/pageSize),pIDs);
            }
          }
        }
      }

      //VERSION TWO (second argument is 2)
      if(version==2){
        //if the switch is on, then there was a pageFault
        if(swt){
          //count up a page fault
          pageFaults++;
          int min, minposition;


          //set up the minimum values with the first frame
          min = pTable[0][counter];
          minposition = 0;
          //get the true minimum value and minimum position
          for(int i=0; i<memFrames; i++){
            if(pTable[i][counter]<min){
              min = pTable[i][counter];
              minposition = i;
            }
          }
          //if the page with minimum value is dirty
          if(pTable[minposition][dirty]==1){
            //look in all the frames
            for(int i=0; i<memFrames &&swt; i++){
              //if there is a page that is not dirty and its counter value is equal to the minimum value
              if(pTable[i][dirty]==0 && pTable[i][counter]==min){
                //then load all the new information
                if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aOperation/pageSize),capturedpID,pTable[i][dirty]);
                pTable[i][pID] = capturedpID;
                pTable[i][page] = aOperation/pageSize;
                pTable[i][ref] = 1; //This variable is not useful in version 2
                pTable[i][dirty] = 0;
                pTable[i][counter] = icont;
                swt = 0;
                //just one disk reference because it was a non dirty page
                diskRef++;
              }
            }
            //if there was no clean page with minimum value, then load new page into the dirty minimum page frame
            if(swt){
              //then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aOperation/pageSize),capturedpID,pTable[i][dirty]);
              pTable[minposition][pID] = capturedpID;
              pTable[minposition][page] = aOperation/pageSize;
              pTable[minposition][ref] = 1; //This variable is not useful in version 2
              pTable[minposition][dirty] = 0;
              pTable[minposition][counter] = icont;
              //two disk references because it was dirty (save) and the new page needed to be loaded
              diskRef = diskRef + 2;
            }
          }
          //if the page with minimum value is not dirty
          else{
            //then load all the new information
            if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aOperation/pageSize),capturedpID,pTable[i][dirty]);
            pTable[minposition][pID] = capturedpID;
            pTable[minposition][page] = aOperation/pageSize;
            pTable[minposition][ref] = 1; //This variable is not useful in version 2
            pTable[minposition][dirty] = 0;
            pTable[minposition][counter] = icont;
            //just one disk reference because it was a non dirty page
            diskRef++;
          }
        }

        //The exact same process but for VALUE ADDRESS now
        //swt is a switch variable to know if a page has fault and been changed
        swt = 1;
        for(int i=0; i<memFrames &&swt; i++){
          //if the page number (aValue/pageSize) of process pIDS is loaded in the frame
          if(pTable[i][pID]==capturedpID && pTable[i][page]==(aValue/pageSize)){
            //Set up the reference bit for this page
            pTable[i][ref] = 1;
            //Set up the counter variable to the current cycle
            pTable[i][counter] = icont;
            //if the action was to write then set up the dirty bit for this page
            if(action=='W') pTable[i][dirty] = 1;
            //turn off the switch swt
            swt = 0;
            //printf("Page %d of pID:%d was in the frame\n",(aValue/pageSize),pIDs);
          }
        }

        if(swt){
          //set up the minimum values with the first frame
          min = pTable[0][counter];
          minposition = 0;
          //get the true minimum value and minimum position
          for(int i=0; i<memFrames; i++){
            if(pTable[i][counter]<min){
              min = pTable[i][counter];
              minposition = i;
            }
          }
          //if the page with minimum value is dirty
          if(pTable[minposition][dirty]==1){
            //look in all the frames
            for(int i=0; i<memFrames &&swt; i++){
              //if there is a page that is not dirty and its counter value is equal to the minimum value
              if(pTable[i][dirty]==0 && pTable[i][counter]==min){
                //then load all the new information
                if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aValue/pageSize),capturedpID,pTable[i][dirty]);
                pTable[i][pID] = capturedpID;
                pTable[i][page] = aValue/pageSize;
                pTable[i][ref] = 1; //This variable is not useful in version 2
                if(action=='W') pTable[i][dirty] = 1;
                pTable[i][counter] = icont;
                swt = 0;
                //just one disk reference because it was a non dirty page
                diskRef++;
              }
            }
            //if there was no clean page with minimum value, then load new page into the dirty minimum page frame
            if(swt){
              //then load all the new information
              if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aValue/pageSize),capturedpID,pTable[i][dirty]);
              pTable[minposition][pID] = capturedpID;
              pTable[minposition][page] = aValue/pageSize;
              pTable[minposition][ref] = 1; //This variable is not useful in version 2
              if(action=='W') pTable[minposition][dirty] = 1;
              pTable[minposition][counter] = icont;
              //two disk references because it was dirty (save) and the new page needed to be loaded
              diskRef = diskRef + 2;
            }
          }
          //if the page with minimum value is not dirty
          else{
            //then load all the new information
            if(debug) printf("Fault! frame:%d page:%d, pID:%d, dirty: %d\n",i+1,(aValue/pageSize),capturedpID,pTable[i][dirty]);
            pTable[minposition][pID] = capturedpID;
            pTable[minposition][page] = aValue/pageSize;
            pTable[minposition][ref] = 1; //This variable is not useful in version 2
            if(action=='W') pTable[minposition][dirty] = 1;
            pTable[minposition][counter] = icont;
            //just one disk reference because it was a non dirty page
            diskRef++;
          }
        }

      }

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
