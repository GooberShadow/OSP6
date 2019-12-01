//memoryManage.c

#include "memoryManage.h"

const key_t ftKey = 0x43214321;
int ftID = 0;
const size_t ftSize = sizeof(FrameTable);
const int FT_OSS_FLAGS = 0666 | IPC_CREAT;
const int FT_USR_FLAGS = 0666;

//-----

void initPageTable(PageTable* pageTable) {
    int i;
    for(i = 0; i < PT_SIZE; ++i) {
        pageTable->table[i] = -1;
    }
}

void initFrameTable(FrameTable* frameTable) {
    int i;
    for(i = 0; i < FT_SIZE; ++i) {
        frameTable->table[i].dirty = 0;
        frameTable->table[i].page = -1;
        frameTable->table[i].pid = -1;
        frameTable->table[i].ref = 0;
        initClock(&frameTable->table[i].timestamp);
    }
}

void destroyPageTable() {
}

void destroyFrameTable() {
    if(ftID > 0)
        shmctl(ftID, IPC_RMID, NULL);
}

int pageFault(FrameTable* frameTable) {
    int oldestIndex = 0;
    int i;
    for(i = 0; i < FT_SIZE - 1; ++i) {
        /* printClock(&frameTable->table[oldestIndex].timestamp); */
        if(checkIfPassedTime(&frameTable->table[oldestIndex].timestamp, &frameTable->table[i + 1].timestamp) == 1) {
            oldestIndex = i;
        }
    }

    return oldestIndex;
}

int addPageToFrameTable(FrameTable* frameTable, long page, int pid, Clock timestamp, long ref) {
    FILE* logger2 = NULL;
    logger2 = fopen("log.txt", "a");
    int pageFaulted = 0;
    //Get a frame
    int frameIndex = getIndexOfFirstEmptyFrame(frameTable);
    if(frameIndex == -1) {
        //fprintf(stderr, "page fault\n");
        pageFaulted = 1;
        frameIndex = pageFault(frameTable);

        //Log page fault
        fprintf (
            logger2, 
            "Master: Address %ld not in a frame, pagefault\n",
            ref
        );

        fprintf (
            logger2, 
            "Master: Clearing frame %d and swapping in %d page %ld\n",
            frameIndex,
            pid,
            page
        );
    } 
    else {
        //Log new page in frame
        fprintf (
            logger2, 
            "Master: Address %ld in frame %d, giving data to %d at time %d:%d\n",
            ref,
            frameIndex,
            pid,
            timestamp.seconds,
            timestamp.nanoseconds
        );
    }

    //Set the frame
    frameTable->table[frameIndex].page = page;
    frameTable->table[frameIndex].pid = pid;
    frameTable->table[frameIndex].ref = ref;
    frameTable->table[frameIndex].dirty = 0;
    initClock(&frameTable->table[frameIndex].timestamp);
    advanceClock (
        &frameTable->table[frameIndex].timestamp,
        timestamp.seconds,
        timestamp.nanoseconds
    );

    fclose(logger2);

    return pageFaulted;
}

void makeDirty(FrameTable* frameTable, long page, int pid) {
    int frameIndex = getIndexOfPageInFrameTable(frameTable, page, pid);
    frameTable->table[frameIndex].dirty = 1;
}

void removePageFromFrameTable(FrameTable* frameTable, long page, int pid) {
    int frameIndex = getIndexOfPageInFrameTable(frameTable, page, pid);
    frameTable->table[frameIndex].dirty = 0;
    frameTable->table[frameIndex].page = -1;
    frameTable->table[frameIndex].pid = -1;
    frameTable->table[frameIndex].ref = 0;
    initClock(&frameTable->table[frameIndex].timestamp);
}

void removePidPagesFromFrameTable(FrameTable* frameTable, int pid) {
    int i;
    for(i = 0; i < FT_SIZE; ++i) {
        if(frameTable->table[i].pid == pid) {
            removePageFromFrameTable(frameTable, frameTable->table[i].page, pid);
        }
    }
}

int getIndexOfPageInFrameTable(FrameTable* frameTable, long page, int pid) {
    int i;
    for(i = 0; i < FT_SIZE; ++i) {
        if (frameTable->table[i].page == page && 
            frameTable->table[i].pid == pid)
        {
            return i;
        }
    }

    return -1;
}

int getIndexOfFirstEmptyFrame(FrameTable* frameTable) {
    int i;
    for(i = 0; i < FT_SIZE; ++i) {
        if(frameTable->table[i].pid == -1) {
            return i;
        }
    }

    return -1;
}

int touchPage(FrameTable* frameTable, long page, int pid, Clock* mainTime, long ref, char* option) {
    FILE* logger3 = NULL;
    int frameIndex = getIndexOfPageInFrameTable(frameTable, page, pid);
    Clock ts;
    setClock(&ts, mainTime->seconds, mainTime->nanoseconds);

    if(frameIndex == -1) {
        return addPageToFrameTable(frameTable, page, pid, ts, ref);
    }

    if(strcmp(option, "LRU") == 0) {
        setClock(&frameTable->table[frameIndex].timestamp, ts.seconds, ts.nanoseconds);
    }

    logger3 = fopen("log.txt", "a");
    
    //Log frame access
    fprintf (
        logger3, 
        "Master: Address %ld in frame %d, giving data to %d at time %d:%d\n",
        ref,
        frameIndex,
        pid,
        ts.seconds,
        ts.nanoseconds
    );

    fclose(logger3);

    return 0;
}

void printFrameTable(FILE* fptr, FrameTable* frameTable) {
    int i;
    for(i = 0; i < FT_SIZE; ++i) {
        fprintf (
            fptr,
            "F#%.3d pid(%.5d) page(%.2ld) ts(%.2d:%.9d), ref(%.2ld), dirt(%d)\n",
            i,
            frameTable->table[i].pid,
            frameTable->table[i].page,
            frameTable->table[i].timestamp.seconds,
            frameTable->table[i].timestamp.nanoseconds,
            frameTable->table[i].ref,
            frameTable->table[i].dirty
        );
    }
}

void printFrame(FrameTable* frameTable, int frameIndex) {
    fprintf (
        stderr,
        "F#%.3d pid(%.5d) page(%.2ld) ts(%.2d:%.9d), ref(%.2ld), dirt(%d)\n",
        frameIndex,
        frameTable->table[frameIndex].pid,
        frameTable->table[frameIndex].page,
        frameTable->table[frameIndex].timestamp.seconds,
        frameTable->table[frameIndex].timestamp.nanoseconds,
        frameTable->table[frameIndex].ref,
        frameTable->table[frameIndex].dirty
    );
}
