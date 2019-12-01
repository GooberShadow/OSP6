//shared.h

#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include "clock.h"
#include "message.h"
#include "processManage.h"
#include "memoryManage.h"

//Keys
extern const key_t SHM_KEY_SEM;     
extern const key_t SHM_KEY_CLOCK;
extern const key_t SHM_KEY_PCB;

//IDs
extern int shmSemID;
extern int shmClockID;
extern int shmPcbID;

//Sizes
extern const size_t shmSemSize;
extern const size_t shmClockSize;
extern const size_t shmPcbSize;

//Flags
extern const int SHM_OSS_FLAGS;
extern const int SHM_USR_FLAGS;

//----------------------------------------

//Shared mem init functions
sem_t* initShmSemaphore(const key_t key, const size_t size, int* shmid, int flags);
void* initSharedMemory(const key_t key, const size_t size, int* shmid, int flags);

//Cleanup functions
void detachAll();
void cleanupSharedMemory(int* shmid);
void cleanupAll();

#endif
