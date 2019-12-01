//usr.c

#include "interrupts.h"
#include "shared.h"

int main(int arg, char* argv[]) {

    //Seed rand
    char* seedOffsetStr = argv[1];
    int seedOffset = atoi(seedOffsetStr);

    srand(time(NULL) + seedOffset);

    //Register signal handler
    usrInitSignalHandler();
    sigaction(SIGTERM, &usrSigAction, 0);
    sigaction(SIGINT, &usrSigAction, 0);

    //Get shared memory pointers
    sem_t* shmSemPtr = 
        initShmSemaphore (
            SHM_KEY_SEM,
            shmSemSize,
            &shmSemID,
            SHM_USR_FLAGS
        );

    Clock* shmClockPtr = 
        (Clock*)initSharedMemory (
            SHM_KEY_CLOCK,
            shmClockSize,
            &shmClockID,
            SHM_USR_FLAGS
        );

    PCB* shmPcbPtr = 
        (PCB*)initSharedMemory (
            SHM_KEY_PCB,
            shmPcbSize,
            &shmPcbID,
            SHM_USR_FLAGS
        );

    //Save spawn time
    Clock spawnTime;
    setClock(&spawnTime, shmClockPtr->seconds, shmClockPtr->nanoseconds);

    //Get PCB index and iterator
    int pcbIndex = getIndexOfPid(shmPcbPtr, getpid());
    PCB* pcbIterator = shmPcbPtr + pcbIndex;

    //Connect message queue
    usrInitMessageQueue();

    //Clock which dictates request times
    Clock reqTime;
    reqTime.nanoseconds = 0;
    reqTime.seconds = 0;

    //Message buffer
    char msgBuff[100];
    int readOrWrite = 0;

    //Requests
    int page, genAddr;
    int processSize = rand() % MAX_PROCESS_SIZE + 1;

    //Stats and death
    int referenceCount = 0;
    int pageFaults = 0;
    Clock totalAccessTime;
    initClock(&totalAccessTime);
    int dieCheck = 1000 + (rand() % 200) - 100;
    int die;
    Clock waitStart, waitStop, waitElapsed, timeDiff;
    initClock(&waitElapsed);

    //-----

    pcbIterator->state = READY;

    while(!usrSignalReceivedFlag) {

        usrReceiveMessage((long)getpid(), &pageFaults);

        //Send request if it is time
        if(checkIfPassedTime(shmClockPtr, &reqTime) == 1) {

            //Death check
            if(referenceCount % dieCheck == 0 && referenceCount > 0) {
                die = rand() % 15;
                fprintf(stderr, "die %d\n", die);
                if(die == 0)
                    break;
            }

            readOrWrite = rand() % 2;
            genAddr = rand() % processSize;
            if(readOrWrite == 0) {
                sprintf(msgBuff, "%d,READ,%d,%d", getpid(), genAddr, genAddr / PAGE_SIZE);
            }
            else {
                sprintf(msgBuff, "%d,WRITE,%d,%d",  getpid(), genAddr, genAddr / PAGE_SIZE);
            }
            
            pcbIterator->state = WAITING;
            setClock(&waitStart, shmClockPtr->seconds, shmClockPtr->nanoseconds);
            usrSendMessage(msgBuff);
            while(pcbIterator->state == WAITING && !usrSignalReceivedFlag);
            setClock(&waitStop, shmClockPtr->seconds, shmClockPtr->nanoseconds);

            //Accumulate wait time
            timeDiff = timeDifference(&waitStop, &waitStart);
            advanceClock(&waitElapsed, timeDiff.seconds, timeDiff.nanoseconds);

            referenceCount++;

            

            /* if(!usrSignalReceivedFlag)
                fprintf(stderr, "%d has made %d refs\n", getpid(), referenceCount); */

            //Generate next request time
            setClock (
                &reqTime,
                shmClockPtr->seconds,
                shmClockPtr->nanoseconds
            );
            advanceClock(&reqTime, 0, rand() % 500000);
        }
    }

    //-----

    //Calculate death statistics:

    double maps = 0;
    double pfpma = 0;
    double amas = 0;

    //maps
    Clock now;
    setClock(&now, shmClockPtr->seconds, shmClockPtr->nanoseconds);
    Clock totalRuntime = timeDifference(&now, &spawnTime);
    double totalTimeFloat = totalRuntime.seconds;
    totalTimeFloat += (double)totalRuntime.nanoseconds / 1000000000.0f;

    maps = (double)referenceCount / totalTimeFloat;

    //pfpma
    pfpma = (double)pageFaults / (double)referenceCount;

    //amas
    double totalWaitTimeFloat = waitElapsed.seconds;
    totalWaitTimeFloat += (double)waitElapsed.nanoseconds / 1000000000.0f;

    amas = totalWaitTimeFloat / (double)referenceCount;

    sprintf(msgBuff, "%f,%f,%f", maps, pfpma, amas);
    sendDeathMessage(msgBuff);

    detachAll();

    return 0;
}
