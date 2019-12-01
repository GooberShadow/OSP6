//oss.c

#include "interrupts.h"
#include "shared.h"

#define TICK_RATE 500            //ns
#define SPAWN_RATE 499999999    //ns

int main(int argc, char* argv[]) {

    //LRU or FIFO
    char* LRUorFIFO = "LRU";
    if(argc > 1) {
        LRUorFIFO = argv[1];
    }

    //Logging
    FILE* logger = NULL;
    logger = fopen("log.txt", "w");
    fclose(logger);

    //Counters and iterators
    int i, j, k;
    PCB* pcbIter;
    
    //Seed rand
    srand(time(NULL));

    //Register signal handlers
    ossInitSignalHandler();
    sigaction(SIGINT, &ossSigAction, 0);
    sigaction(SIGALRM, &ossSigAction, 0);
    sigaction(SIGTERM, &ossSigAction, 0);

    //Get semaphore pointer
    sem_t* shmSemPtr = 
        initShmSemaphore (
            SHM_KEY_SEM, 
            shmSemSize, 
            &shmSemID, 
            SHM_OSS_FLAGS
        );
    
    //Init semaphore
    if(sem_init(shmSemPtr, 1, 1) == -1) {
        perror("ERROR:sem_init failed");
        cleanupAll();
        exit(1);
    }

    //Get shared memory pointers
    Clock* shmClockPtr = 
        (Clock*)initSharedMemory (
            SHM_KEY_CLOCK, 
            shmClockSize, 
            &shmClockID, 
            SHM_OSS_FLAGS
        );

    PCB* shmPcbPtr = 
        (PCB*)initSharedMemory (
            SHM_KEY_PCB, 
            shmPcbSize, 
            &shmPcbID, 
            SHM_OSS_FLAGS
        );
    
    FrameTable* shmFrameTable = 
        (FrameTable*)initSharedMemory (
            ftKey,
            ftSize,
            &ftID,
            FT_OSS_FLAGS
        );

    //Init shared memory values
    initClock(shmClockPtr);
    ossInitPcbArray(shmPcbPtr);
    initFrameTable(shmFrameTable);

    //Message queue init
    int msgPid, msgReqType, msgReqAddr, msgPage;
    Clock timestamp;
    char msgBuff[100];
    ossInitMessageQueue();

    //Generate first process random spawn time
    Clock spawnTime;
    spawnTime.nanoseconds = rand() % 499999999 + 1;
    spawnTime.seconds = 0;

    //Mark every second
    int prevSecond = shmClockPtr->seconds;

    //Child death stats
    double accessPerSecond, faultsPerAccess, avgAccessSpeed;

    int pageFaulted = 0;
    int extraTime = 0;

    //-----

    while(!ossSignalReceivedFlag) {

        if(shmClockPtr->seconds > prevSecond) {
            fprintf(stderr, "\n\n");
            printClock(shmClockPtr);
            fprintf(stderr, "\n\n");
            prevSecond = shmClockPtr->seconds;

            //Log frame table each "second"
            logger = fopen("log.txt", "a");
            fprintf (
                logger, 
                "\nFRAME TABLE @ Time(%d:%d)\n",
                shmClockPtr->seconds,
                shmClockPtr->nanoseconds
            );
            printFrameTable(logger, shmFrameTable);
            fprintf(logger, "\n");
            fclose(logger);

            sleep(2);
        }

        //Process message
        msgPid = -1;
        msgReqType = -1;
        msgReqAddr = -1;
        msgPage = -1;
        extraTime = 0;
        ossReceiveMessage(&msgPid, &msgReqType, &msgReqAddr, &msgPage);        

        if(msgPid != -1) {

            //Log request
            logger = fopen("log.txt", "a");
            if(msgReqType == READ) {
                fprintf (
                    logger, 
                    "Master: %d requesting READ of addr %d at time %d:%d\n",
                    msgPid,
                    msgReqAddr,
                    shmClockPtr->seconds,
                    shmClockPtr->nanoseconds
                );
            }
            else {
                fprintf (
                    logger, 
                    "Master: %d requesting WRITE of addr %d at time %d:%d\n",
                    msgPid,
                    msgReqAddr,
                    shmClockPtr->seconds,
                    shmClockPtr->nanoseconds
                );
            }
            fclose(logger);

            setClock(&timestamp, shmClockPtr->seconds, shmClockPtr->nanoseconds);
            pageFaulted = touchPage(shmFrameTable, msgPage, msgPid, &timestamp, msgReqAddr, LRUorFIFO);

            if(pageFaulted) {
                extraTime += 14000000;
            }
            
            pcbIter = shmPcbPtr;
            pcbIter += getIndexOfPid(shmPcbPtr, msgPid);
            if(pcbIter->state == WAITING) {             
                if(msgReqType == READ) {
                    sprintf (
                        msgBuff, 
                        "%d,APPROVED,READ,%d",
                        msgPid,
                        pageFaulted
                    );
                }
                else {
                    makeDirty(shmFrameTable, msgPage, msgPid);

                    logger = fopen("log.txt", "a");
                    fprintf (
                        logger, 
                        "Master: Dirty bit of frame %d set, adding additional time to the clock\n",
                        getIndexOfPageInFrameTable(shmFrameTable, msgPage, msgPid)
                    );
                    fclose(logger);
                    extraTime += 50;
                    sprintf (
                        msgBuff,
                        "%d,APPROVED,WRITE,%d",
                        msgPid,
                        pageFaulted
                    );
                }
                ossSendMessage(pcbIter->pid, msgBuff);
                pcbIter->state = READY;
            }
        }
        

        //Spawn process every 500ms
        if(checkIfPassedTime(shmClockPtr, &spawnTime)) {
            spawnProcess(shmPcbPtr);

            //Generate next spawn time
            setClock (
                &spawnTime,
                shmClockPtr->seconds,
                shmClockPtr->nanoseconds
            );
            advanceClock(&spawnTime, 0, rand() % SPAWN_RATE + 1);
        }

        //Advance the clock
        sem_wait(shmSemPtr);
            advanceClock(shmClockPtr, 0, TICK_RATE + extraTime);
        sem_post(shmSemPtr);

        //Wait on dead child if there is one
        waitNoBlock(shmPcbPtr, shmFrameTable, &accessPerSecond, &faultsPerAccess, &avgAccessSpeed);
    }

    //-----

    //Signal too all child processes to finish up
    killChildren(shmPcbPtr);

    //Wait on remaining processes
    while(areActiveProcesses(shmPcbPtr) == 1)
        waitNoBlock(shmPcbPtr, shmFrameTable, &accessPerSecond, &faultsPerAccess, &avgAccessSpeed);

    //Cleanup shared memory and message queue
    cleanupAll();
}
