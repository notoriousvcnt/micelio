#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

class RandomUrn {
public:
    RandomUrn(int min, int max) : min(min), max(max), currentIndex(0) {
        size = max - min + 1;
        for (int i = 0; i < size; ++i) {
            numbers[i] = min + i;
        }
        shuffleNumbers();
    }

    int getNext() {
        if (currentIndex >= size) {
            shuffleNumbers();
            currentIndex = 0;
        }
        return numbers[currentIndex++];
    }

private:
    void shuffleNumbers() {
        for (int i = 0; i < size; ++i) {
            int j = random(i, size);
            int temp = numbers[i];
            numbers[i] = numbers[j];
            numbers[j] = temp;
        }
    }

    int numbers[100]; // Adjust the size as needed
    int min, max, size;
    int currentIndex;
};

#endif // UTILS_H
