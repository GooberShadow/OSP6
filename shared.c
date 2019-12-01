//shared.c

#include "shared.h"

//Keys
const key_t SHM_KEY_SEM = 0x66666666;   
const key_t SHM_KEY_CLOCK = 0x99999999;
const key_t SHM_KEY_PCB = 0x77777777;

//IDs
int shmSemID = 0;
int shmClockID = 0;
int shmPcbID = 0;

//Sizes
const size_t shmSemSize = sizeof(sem_t);
const size_t shmClockSize = sizeof(Clock);
const size_t shmPcbSize = MAX_CHILD_PROCESSES * sizeof(PCB);

//Flags
const int SHM_OSS_FLAGS = IPC_CREAT | IPC_EXCL | 0777;
const int SHM_USR_FLAGS = 0777;

//-----

sem_t* initShmSemaphore(const key_t key, const size_t size, int* shmid, int flags) {
    //Allocate shared memory and get an id
    *shmid = shmget(key, size, flags);
    if(*shmid < 0) {
        perror("ERROR:shmget failed(semaphore)");
        cleanupAll();
        exit(1);
    }

    //Assign pointer
    void* temp = shmat(*shmid, NULL, 0);
    if(temp == (void*) -1) {
        perror("ERROR:shmat failed(semaphore)");
        cleanupAll();
        exit(1);
    }

    return (sem_t*)temp;
}

void* initSharedMemory(const key_t key, const size_t size, int* shmid, int flags) {
    //Allocate shared memory and get an id
    *shmid = shmget(key, size, flags);
    if(*shmid < 0) {
        if(key == SHM_KEY_CLOCK) {
            perror("ERROR:shmid failed(clock)");
        }
        cleanupAll();
        exit(10);
    }

    //Assign pointer
    void* temp = shmat(*shmid, NULL, 0);
    if(temp == (void*) -1) {
        if(key == SHM_KEY_CLOCK) { 
            perror("ERROR:oss:shmat failed(clock)");
        }
        cleanupAll();
        exit(20);
    }

    return temp;
}

void detachAll() {
    if(shmClockID > 0)
        shmdt(&shmClockID);
    if(shmSemID > 0)
        shmdt(&shmSemID);
    if(shmPcbID > 0)
        shmdt(&shmPcbID);
}

void cleanupSharedMemory(int* shmid) {
    int cmd = IPC_RMID;
    int rtrn = shmctl(*shmid, cmd, NULL);
    if(rtrn == -1) {
        perror("ERROR:oss:shmctl failed");
        exit(1);
    }
}

void cleanupAll() {
    if(shmSemID > 0)
        cleanupSharedMemory(&shmSemID);
    if(shmClockID > 0)   
        cleanupSharedMemory(&shmClockID);
    if(shmPcbID > 0)   
        cleanupSharedMemory(&shmPcbID);

    destroyMessageQueue();
    destroyFrameTable();
}
