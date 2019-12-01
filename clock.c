//clock.c

#include "clock.h"

void setClock(Clock* clock, unsigned int sec, unsigned int nanosec) {
    if(nanosec >= 1000000000) {
        fprintf(stderr, "ERROR: Invalid clock set parameters\n");
        return;
    }

    clock->seconds = sec;
    clock->nanoseconds = nanosec;
    return;
}

void advanceClock(Clock* mainClock, unsigned int sec, unsigned int nanosec) {
    //Subtract seconds off of nanoseconds if >= 1,000,000,000 
    while(nanosec >= 1000000000) {
        ++sec;
        nanosec -= 1000000000;
    }

    //Set the clock
    if(mainClock->nanoseconds + nanosec < 1000000000) {
        mainClock->nanoseconds += nanosec;
    }
    else {
        ++sec;
        mainClock->nanoseconds = nanosec - (1000000000 - mainClock->nanoseconds);
    }
    
    mainClock->seconds += sec;
}

void initClock(Clock* clock){
    clock->seconds = 0;
    clock->nanoseconds = 0;
}

void printClock(Clock* clock) {
    if(clock != NULL)
        fprintf(stderr, "Clock Reads %d:%.9d\n", clock->seconds, clock->nanoseconds);
    else
        fprintf(stderr, "Clock is null\n");
}

Clock timeDifference(Clock* minuend, Clock* subtrahend) {
    int secondsDifference = 0;
    int nanoDifference = 0;

    secondsDifference = minuend->seconds - subtrahend->seconds;
    nanoDifference = minuend->nanoseconds - subtrahend->nanoseconds;

    if(nanoDifference < 0) {
        --secondsDifference;
        nanoDifference = 1000000000 + nanoDifference;
    }

    Clock rtrnClock;
    rtrnClock.nanoseconds = nanoDifference;
    rtrnClock.seconds = secondsDifference;

    return rtrnClock;
}

int checkIfPassedTime(Clock* mainClock, Clock* timeLimit) {
    if(mainClock->seconds > timeLimit->seconds)
        return 1;

    if(mainClock->seconds == timeLimit->seconds && 
        mainClock->nanoseconds >= timeLimit->nanoseconds)
    {
        return 1;
    }
    
    return 0;
}
