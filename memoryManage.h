//memoryManage.h

#ifndef MEM_MAN_H
#define MEM_MAN_H

#define TOT_SYS_MEM 262144      //256 * 1024 (256K)
#define MAX_PROCESS_SIZE 32768  //32 * 1024 (32K)
#define PAGE_SIZE 1024          //(1K)

#define PT_SIZE 32
#define FT_SIZE 256

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

//Shared memory vars
extern const key_t ftKey;
extern int ftID;
extern const size_t ftSize;
extern const int FT_OSS_FLAGS;
extern const int FT_USR_FLAGS;

typedef struct page_tbl_struct {
    long table[PT_SIZE];
} PageTable;

typedef struct frame_struct {
    long page;
    int pid;
    Clock timestamp;
    int dirty;
    long ref;
} Frame;

typedef struct frame_tbl_struct {
    Frame table[FT_SIZE];
} FrameTable;

//Inits
void initPageTable(PageTable* pageTable);
void initFrameTable(FrameTable* frameTable);
void destroyPageTable();
void destroyFrameTable();

//Managers
int pageFault(FrameTable* frameTable);
int addPageToFrameTable(FrameTable* frameTable, long page, int pid, Clock timestamp, long ref);
void makeDirty(FrameTable* frameTable, long page, int pid);
void removePageFromFrameTable(FrameTable* frameTable, long page, int pid);
void removePidPagesFromFrameTable(FrameTable* frameTable, int pid);
int touchPage(FrameTable* frameTable, long page, int pid, Clock* mainTime, long ref, char* option);

//Utility
int getIndexOfPageInFrameTable(FrameTable* frameTable, long page, int pid);
int getIndexOfFirstEmptyFrame(FrameTable* frameTable);
void printFrameTable(FILE* fptr, FrameTable* frameTable);
void printFrame(FrameTable* frameTable, int frameIndex);

#endif
