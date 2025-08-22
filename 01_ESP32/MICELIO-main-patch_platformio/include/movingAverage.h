#ifndef MOVINGAVERAGE_H
#define MOVINGAVERAGE_H
#include  <Arduino.h>

//moving average filter
int movingAverage(int *array, int length) {
  int sum = 0;
  for (int i = 0; i < length; i++) {
    sum += array[i];
  }
  return sum / length;
}

// Function to update the distance array and calculate the moving average
void updateDistanceArray(int *array, int newDistance) {
  // Shift all elements to the left
  for (int i = 1; i < 10; i++) {
    array[i - 1] = array[i];
  }
  // Add the new distance to the end of the array
  array[9] = newDistance;
}

// Function to get the filtered distance using moving average
int getFilteredDistance(int *array, int meanAmount) {
  return movingAverage(array, meanAmount);
}

#endif