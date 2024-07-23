#ifndef MyClass_h
#define MyClass_h

#include "Arduino.h"

class Timer {
  public:
    Timer();
    void startTimer(int timerInterval);
    int checkTimer();
    int remainingTime();  
  private:
    int _timerStartTime;
    int _timerInterval;
};
#endif
