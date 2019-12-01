//clock.h

#ifndef CLOCK_H
#define CLOCK_H

#include <stdio.h>

typedef struct clock_struct {
    unsigned int seconds;
    unsigned int nanoseconds;
} Clock;

void initClock(Clock* clock);
void setClock(Clock* clock, unsigned int sec, unsigned int nanosec);
void advanceClock(Clock* mainClock, unsigned int sec, unsigned int nanosec);
Clock timeDifference(Clock* subtractFrom, Clock* subtractAmount);
void printClock(Clock* clock);
int checkIfPassedTime(Clock* mainClock, Clock* timeLimit);

#endif
