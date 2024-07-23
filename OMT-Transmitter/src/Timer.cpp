#include "Arduino.h"
#include "Timer.h"

//Constructor Deffinition.  executes whenever an object is created
Timer::Timer() {
  _timerStartTime = millis();
}

void Timer::startTimer(int timerInterval) {
  _timerInterval = timerInterval;
  _timerStartTime = millis();
}

int Timer::checkTimer() {
  if (_timerInterval == 0) {
    return 1;
  }

  unsigned long currentTime = millis();
  if (currentTime - _timerStartTime >= _timerInterval) {
    _timerInterval = 0;
    return 0; // Timer complete
  }

  return 1; // Timer active
}

int Timer::remainingTime() {
  if (_timerInterval == 0) {
    return 0; // Timer not active
  }

  unsigned long currentTime = millis();
  if (currentTime - _timerStartTime <= _timerInterval) {
    return _timerInterval - (currentTime - _timerStartTime);
  }

  return 0;
}