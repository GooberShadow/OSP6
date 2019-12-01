//processManage.c

#include "processManage.h"

static pid_t pid = 0;
static int exitStatus = 0;

//Static functions:

static PCB* scanForEmptyPcbSlot(PCB* pcbArray) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't scan for empty--nullptr\n");
        return NULL;
    }

    PCB* iterator = pcbArray;
    int i;
    for(i = 0; i < MAX_CHILD_PROCESSES; ++i) {
        if(iterator->state == NULL_PS){
            return iterator;
        }
        iterator++;
    }

    return NULL;
}

static int pcbArrayFull(PCB* pcbArray) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't check if full--nullptr\n");
        return -1;
    }

    PCB* iterator = pcbArray;
    int i;
    for(i = 0; i < MAX_CHILD_PROCESSES; ++i) {
        if(iterator->state == NULL_PS) {
            return 0;
        }
        iterator++;
    }

    return 1;
}

int areActiveProcesses(PCB* pcbArray) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't check for processes--nullptr\n");
        return -1;
    }

    PCB* iterator = pcbArray;
    int i;
    for(i = 0;i < MAX_CHILD_PROCESSES; ++i) {
        if(iterator->state == READY) {
            return 1;
        }
        iterator++;
    }

    return 0;
}

//Initialization/deallocation:

void initPcb(PCB* pcbIterator) {
    pcbIterator->pid = 0;
    pcbIterator->state = NULL_PS;
}

void ossInitPcbArray(PCB* pcbArray) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: PCB Array init--nullptr\n");
        return;
    }

    PCB* iterator = pcbArray;
    int i;
    for(i = 0; i < MAX_CHILD_PROCESSES; ++i) {
        initPcb(iterator);
        iterator++;
    }
}

//oss functions:

int spawnProcess(PCB* pcbArray) {

    if(pcbArrayFull(pcbArray) == 1) {
        return 0;
    }

    pid = fork();

    //rand() was not random at all w/out this
    int randSeedOffset = rand() % 100;
    char rso[5];
    sprintf(rso, "%d", randSeedOffset);

    //Error
    if(pid < 0) {
        fprintf(stderr, "ERROR: Fork\n");
        return -1;
    }

    //Child
    if(pid == 0) {
        execl("./usr", "usr", rso, (char*) NULL);
    }

    addToPcbArray(pcbArray, pid);
    pid = 0;

    return 1;
}

void addToPcbArray(PCB* pcbArray, pid_t pid) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't add ps to PCB Array--nullptr\n");
        return;
    }

    if(pid <= 0) {
        fprintf(stderr, "ERROR: Couldn't add ps to PCB Array--Invalid PID\n");
        return;
    }

    PCB* iterator = scanForEmptyPcbSlot(pcbArray);
    if(iterator == NULL) {
        fprintf(stderr, "Couldn't add ps to PCB Array--No Empty\n");
        return;
    }

    iterator->pid = pid;
    iterator->state = INITIALIZING;
}

void waitNoBlock(PCB* pcbArray, FrameTable* frameTable, double* accessPerSecond, double* faultsPerAccess, double* avgAccessSpeed) {
    while((pid = waitpid(-1, &exitStatus, WNOHANG))) {
        if((pid == -1) && (errno != EINTR))
            break;
        else {
            fprintf(
                stderr, 
                "EXIT pid(%d) w/ status %d\n", 
                pid, WEXITSTATUS(exitStatus)
            );
            
            receiveDeathMessage(accessPerSecond, faultsPerAccess, avgAccessSpeed);

            //Log death stats
            FILE* logger4 = NULL;
            logger4 = fopen("log.txt", "a");
            fprintf (
                logger4,
                "\nDEATH: %d\n\tAccesses/second: %f\n\tFaults/Access: %f\n\t Avg Mem Access Speed: %f\n\n",
                pid,
                *accessPerSecond,
                *faultsPerAccess,
                *avgAccessSpeed
            );
            fclose(logger4);

            removeFromPcbArray(pcbArray, pid);
            removePidPagesFromFrameTable(frameTable, pid);
        }
    }
}

void removeFromPcbArray(PCB* pcbArray, pid_t pid) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't remove ps from PCB Array--nullptr\n");
        return;
    }

    if(pid <= 0) {
        fprintf(stderr, "ERROR: Couldn't remove ps from PCB Array--Invalid PID\n");
        return;
    }

    //Scan PCB array for pid
    PCB* iterator = pcbArray;
    int i;
    for(i = 0; i < MAX_CHILD_PROCESSES; ++i) {
        if(iterator->pid == pid) {
            iterator->state = NULL_PS;
            iterator->pid = 0;
            return;
        }
        iterator++;
    }

    fprintf(stderr, "PID %d not found in PCB Array--No Remove\n", (int)pid);
    return;
}

void killChildren(PCB* pcbArray) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: No children were killed--nullptr\n");
        return;
    }

    PCB* iterator = pcbArray;
    int i;
    for(i = 0; i < MAX_CHILD_PROCESSES; ++i) {
        if(iterator->state != NULL_PS) {
            kill(iterator->pid, SIGTERM);
        }
        iterator++;
    }
}

//Utility:
void printPcb(PCB* pcbArray, int position) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't print PCB--nullptr\n");
        return;
    }

    PCB*  iterator = (pcbArray + position);

    fprintf(stderr, "PCB#%.2d: pid(%.5d), state(%d)\n",
        position,
        (int)iterator->pid,
        (int)iterator->state
    );
}

void printPcbArray(PCB* pcbArray) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't print PCB array--nullptr\n");
        return;
    }

    PCB* iterator = pcbArray;
    int i;
    for(i = 0; i < MAX_CHILD_PROCESSES; ++i) {
        fprintf(stderr, "PCB#%.2d: pid(%.5d), state(%d)\n", 
            i, 
            (int)iterator->pid,
            (int)iterator->state
        );
        iterator++;
    }

}

int getIndexOfPid(PCB* pcbArray, pid_t pid) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't fetch index of pid--nullptr\n");
        return -1;
    }

    if((int)pid <= 0) {
        fprintf(stderr, "ERROR: Couldn't fetch index of pid--invalid pid\n");
        return -1;
    }

    PCB* iterator = pcbArray;
    int i;
    for(i = 0; i < MAX_CHILD_PROCESSES; ++i) {
        if(iterator->pid == pid) {
            return i;
        }
        iterator++;
    }

    return -1;
}

int spawnDummyProcess(PCB* pcbArray, pid_t pid) {
    if(pcbArray == NULL) {
        fprintf(stderr, "ERROR: Couldn't spawn dummy--nullptr\n");
        return -1;
    }

    PCB* iterator = scanForEmptyPcbSlot(pcbArray);
    if(iterator == NULL) {
        fprintf(stderr, "ERROR: Couldn't spawn dummy--No Empty\n");
        return -1;
    }

    iterator->state = READY;
    iterator->pid = pid;
}
