//message.h

#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#include "clock.h"

#define MAX_MSG_LEN 100

struct Msg {
    long msgType;
    char msgText[MAX_MSG_LEN];
};

typedef enum req_type_enum {
    READ,
    WRITE
} reqType;

typedef enum msg_type_enum {
    TO_OSS = 100000,
    DEATH_MSG = 100001
} msgType;

//Shared memory vars
extern const key_t msgKey;
extern int msgID;
extern const size_t msgSize;
extern const int MSG_OSS_FLAGS;
extern const int MSG_USR_FLAGS;

//Inits and destroys
void ossInitMessageQueue();
void usrInitMessageQueue();
void destroyMessageQueue();

//Send/Receive
void ossSendMessage(long pid, const char* text);
void ossReceiveMessage(int* msgPid, int* msgReqType, int* msgReqAddr, int* msgPage);
void usrSendMessage(const char* text);
void usrReceiveMessage(long pid, int* faultCounter);

void sendDeathMessage(const char* text);
void receiveDeathMessage(double* accessPerSecond, double* faultsPerAccess, double* avgAccessSpeed);

#endif
