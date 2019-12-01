//interrupts.c

#include "interrupts.h"

//Global signal flags
int ossSignalReceivedFlag = 0;
int usrSignalReceivedFlag = 0;
int ossPauseFlag = 0;

//Global sigaction structs
struct sigaction ossSigAction;
struct sigaction usrSigAction;

void ossSignalHandler(int signal) {
    switch(signal) {
        case SIGINT:
            fprintf(stderr, "\nOSS caught SIGINT signal\n");
            ossSignalReceivedFlag = 1;
            break;

        case SIGALRM:
            fprintf(stderr, "OSS caught SIGALRM signal\n");
            ossSignalReceivedFlag = 1;
            break;
        
        case SIGTERM:
            ossPauseFlag = 1;
            break;
    }    
}

void usrSignalHandler(int signal) {
    switch(signal) {
        case SIGTERM:
            fprintf(stderr, "USR %d caught SIGTERM signal\n", getpid());
            usrSignalReceivedFlag = 1;
            break;
    } 
}

void ossInitSignalHandler() {
    memset(&ossSigAction, 0, sizeof(ossSigAction));
    ossSigAction.sa_handler = ossSignalHandler;
}

void usrInitSignalHandler() {
    memset(&usrSigAction, 0, sizeof(usrSigAction));
    usrSigAction.sa_handler = usrSignalHandler;
}
