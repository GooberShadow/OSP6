//message.c

#include "message.h"

//Shared memory vars
const key_t msgKey = 0x12341234;
int msgID = 0;
const size_t msgSize = sizeof(struct Msg) - sizeof(long);
const int MSG_OSS_FLAGS = 0666 | IPC_CREAT;
const int MSG_USR_FLAGS = 0666;

//Inits/destroys:

void ossInitMessageQueue() {
    msgID = msgget(msgKey, MSG_OSS_FLAGS);
    if(msgID == -1) {
        perror("ERROR: msgget");
    }
}

void usrInitMessageQueue() {
    msgID = msgget(msgKey, MSG_USR_FLAGS);
    if(msgID == -1) {
        perror("ERROR: msgget");
    }
}

void destroyMessageQueue() {
    if(msgID > 0)
        msgctl(msgID, IPC_RMID, NULL);
}

//Send/receive:

void ossSendMessage(long pid, const char* text) {
    struct Msg newMsg;
    int len = strlen(text) + 1;
    newMsg.msgType = pid;
    memcpy((char*)newMsg.msgText, (char*)text, len);
    msgsnd(msgID, &newMsg, len, 0);
}

void ossReceiveMessage(int* msgPid, int* msgReqType, int* msgReqAddr, int* msgPage) {
    struct Msg rcvMsg;
    int i;
    char* token = NULL;
    int success = 
        msgrcv (
            msgID,
            &rcvMsg, 
            sizeof(rcvMsg.msgText), 
            TO_OSS, 
            IPC_NOWAIT
        );
    
    if(success != -1) {
        //Parse the message
        token = strtok(rcvMsg.msgText, ",");
        *msgPid = atoi(token);
        for(i = 0; i < 3; ++i){
            token = strtok(NULL, ",");
            switch(i) {
                case 0:
                    if(strcmp(token, "READ") == 0) {
                        *msgReqType = READ;
                    }
                    else if(strcmp(token, "WRITE") == 0) {
                        *msgReqType = WRITE;
                    }
                break;

                case 1:
                    *msgReqAddr = atoi(token);
                break;

                case 2:
                    *msgPage = atoi(token);
                break;
            }
        }

        fprintf (
            stderr, 
            "OSS rcv: %d %d %d %d\n", 
            *msgPid, *msgReqType, *msgReqAddr, *msgPage
        );
    }    
}

void usrSendMessage(const char* text) {
    struct Msg newMsg;
    newMsg.msgType = TO_OSS;
    int len = strlen(text) + 1;
    memcpy((char*)newMsg.msgText, (char*)text, len);
    msgsnd(msgID, &newMsg, len, 0);
}

void usrReceiveMessage(long pid, int* faultCounter) {
    struct Msg rcvMsg;
    int i, read;
    char* token = NULL;
    int success = msgrcv(msgID, &rcvMsg, sizeof(rcvMsg.msgText), pid, IPC_NOWAIT);
    if(success != -1) {
        //Parse the message
        token = strtok(rcvMsg.msgText, ",");
        for(i = 0; i < 3; ++i){
            token = strtok(NULL, ",");
            switch(i) {
                case 1:
                    if(strcmp(token, "READ") == 0) {
                        read = READ;
                    }
                    else if(strcmp(token, "WRITE") == 0) {
                        read = WRITE;
                    }
                break;

                case 2:
                    if(atoi(token) == 1)
                        (*faultCounter) += 1;
                break;
            }
        }

        fprintf (
            stderr,
            "USR %ld rcv: APPROVED READ/WRITE(%d) PF(%d)\n",
            pid, read, *faultCounter
        );
    }
}

void sendDeathMessage(const char* text) {
    struct Msg newMsg;
    newMsg.msgType = DEATH_MSG;
    int len = strlen(text) + 1;
    memcpy((char*)newMsg.msgText, (char*)text, len);
    msgsnd(msgID, &newMsg, len, 0);
}

void receiveDeathMessage(double* accessPerSecond, double* faultsPerAccess, double* avgAccessSpeed) {
    struct Msg rcvMsg;
    int i;
    char* token = NULL;
    int success = 
        msgrcv (
            msgID,
            &rcvMsg, 
            sizeof(rcvMsg.msgText), 
            DEATH_MSG, 
            IPC_NOWAIT
        );
    
    if(success != -1) {
        //Parse the message
        token = strtok(rcvMsg.msgText, ",");
        *accessPerSecond = atof(token);
        for(i = 0; i < 3; ++i){
            token = strtok(NULL, ",");
            switch(i) {
                case 0:
                    *faultsPerAccess = atof(token);
                break;

                case 1:
                    *avgAccessSpeed = atof(token);
                break;
            }
        }

        fprintf (
            stderr, 
            "DEATH: %f %f %f\n", 
            *accessPerSecond, 
            *faultsPerAccess, 
            *avgAccessSpeed
        );
    }
}
