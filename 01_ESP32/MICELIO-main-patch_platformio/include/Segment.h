#ifndef SEGMENT_H
#define SEGMENT_H

#include <Arduino.h>
#include <FastLED.h>

class Segment {
public:
    Segment(CRGB* leds, int start, int end, int controllerIndex);
    void setColor(CRGB color);
    void show();
    void pulseAnimation(int frameCount, int speed, int blobSize, CRGB color, int fadePercent);
    void breathingBrightness(int frameCount, int minBrightness, int maxBrightness, int speed, int fadeAmount);
    void resetPulsePosition() { pulsePosition = 0; start = 0; }
    void resetBrightness(int value) { brightness = value; }
    int pulseAnimation2(int speed, int blobSize, CRGB color, int fadePercent, bool reset);
    
    int brightness = 255;
    int ledPos;
    int prevLedPos;
    uint32_t localBasetime;
    bool finishedLoop = true;
private:
    CRGB* leds;
    int start;
    int end;
    int numLeds;
    int controllerIndex;
    unsigned long lastFrame = 0;
    int pulsePosition = 0;
    
};

Segment::Segment(CRGB* leds, int start, int end, int controllerIndex) : leds(leds), start(start), end(end), controllerIndex(controllerIndex){
    numLeds = end - start + 1;

}

void Segment::setColor(CRGB color) {
  fill_solid(&leds[start], numLeds, color);
}

void Segment::show() {
    FastLED.show();
}


void Segment::pulseAnimation(int frameCount, int speed, int blobSize, CRGB color, int fadePercent) {
  float fadeAmount = 255 * fadePercent / 100.0;
  if (frameCount - lastFrame > speed){
    lastFrame = frameCount;
    pulsePosition = (pulsePosition + 1) % (numLeds - blobSize);
    
    for (int i = pulsePosition; i < pulsePosition + blobSize; i++) {
      leds[start+i] = color;
    }
    fadeToBlackBy(leds, numLeds, int(fadeAmount));
    
  }
  
}

int Segment::pulseAnimation2(int speed, int blobSize, CRGB color, int fadePercent, bool reset) {
  float fadeAmount = 255 * fadePercent / 100.0;
  //Serial.println(fadeAmount);
  fadeToBlackBy(leds, numLeds, int(fadeAmount));
  if (reset){
    localBasetime = millis();
  } 
  prevLedPos = ledPos;
  int pos = beat16(speed, localBasetime);
  ledPos = map(pos, 0, 65535, 0, numLeds);

  for (int i = 0; i < blobSize; i++) {
      leds[ledPos+i] = color;
    }
  
  return ledPos-prevLedPos;
}

void Segment::breathingBrightness(int frameCount, int minBrightness, int maxBrightness, int speed, int fadeAmount){
  brightness = dim8_lin(maxBrightness*sin8(frameCount * speed)/255 + minBrightness);
} 



/* ----------- LED Strips functions ----------- 

// void pulseAnimation(CRGB* ledsArray, int numLeds, int speedInMs, int blobSize = 1, CRGB color = CRGB::DarkOrchid, int fadePercent = 25) {
  static int timer = millis();
  float fadeAmount = 256 * fadePercent / 100;
  if (millis() - timer > speedInMs){
    timer = millis();
    pulsePosition = (pulsePosition + 1) % (NUM_LEDS - blobSize);
    
    for (int i = pulsePosition; i < pulsePosition + blobSize; i++) {
        ledsArray[i] = color;
    }

    fadeToBlackBy(ledsArray, numLeds, fadeAmount);
    //leds(pulsePosition,pulsePosition+ blobSize) = CRGB::Black;
    pulsePosition = (pulsePosition + 1) % (NUM_LEDS- blobSize);
    FastLED.show(); 
  }
}

void pulseAnimationSegment(CRGB* ledsArray, int startLed, int numLeds, int speed, int blobSize = 1, CRGB color = CRGB::DarkOrchid, int fadePercent = 25) {
  float fadeAmount = 256 * fadePercent / 100;
  if (frameCount - lastFrame > speed){
    lastFrame = frameCount;
    pulsePosition = (pulsePosition + 1) % (numLeds - blobSize);
    
    for (int i = pulsePosition; i < pulsePosition + blobSize; i++) {
      ledsArray[startLed+i] = color;
    }

    fadeToBlackBy(&ledsArray[startLed], numLeds, fadeAmount);
  }
}

void pulseAnimationSegment2(CRGB* ledsArray, unsigned long &lastFrame, int startLed, int numLeds, int speed, int blobSize = 1, CRGB color = CRGB::DarkOrchid, int fadePercent = 25) {
  float fadeAmount = 256 * fadePercent / 100;
  if (frameCount - lastFrame > speed){
    lastFrame = frameCount;
    pulsePosition2 = (pulsePosition + 1) % (numLeds - blobSize);
    
    for (int i = pulsePosition; i < pulsePosition + blobSize; i++) {
      ledsArray[startLed+i] = color;
    }

    fadeToBlackBy(&ledsArray[startLed], numLeds, fadeAmount);
  }
}

void pulseAnimationGradient(CRGB* ledsArray, CRGB* gradientArray, int numLeds, int speedInMs, int blobSize = 1, int fadePercent = 25) {
  static int timer = millis();
  float fadeAmount = 256 * fadePercent / 100;
  if (millis() - timer > speedInMs){
    timer = millis();
    //fadeToBlackBy(leds(pulsePosition,pulsePosition + blobSize), NUM_LEDS, fadeAmount);
    pulsePosition = (pulsePosition + 1) % (NUM_LEDS - blobSize);
    for (int i = pulsePosition; i < pulsePosition + blobSize; i++) {
        ledsArray[i] = gradientArray[i];
    }

    fadeToBlackBy(ledsArray, numLeds, fadeAmount);
    
    //leds(pulsePosition,pulsePosition+ blobSize) = CRGB::Black;
   //pulsePosition = (pulsePosition + 1) % (NUM_LEDS- blobSize);
    //FastLED.show(); 
  }
}

void pulseAnimationGradientSegment(CRGB* ledsArray,int controllerIndex, int startLed, int numLeds, CRGB* gradientArray, int speedInMs, int blobSize = 1, int fadePercent = 25) {
  static int timer1 = millis();
  float fadeAmount = 256 * fadePercent / 100;
  if (millis() - timer1 > speedInMs){
    timer1 = millis();
    //fadeToBlackBy(leds(pulsePosition,pulsePosition + blobSize), NUM_LEDS, fadeAmount);
    pulsePosition = (pulsePosition + 1) % (numLeds - blobSize);
    for (int i = pulsePosition; i < pulsePosition + blobSize; i++) {
        ledsArray[startLed+i] = gradientArray[i];
    }

    fadeToBlackBy(&ledsArray[startLed], numLeds, fadeAmount);
    
    //leds(pulsePosition,pulsePosition+ blobSize) = CRGB::Black;
   //pulsePosition = (pulsePosition + 1) % (NUM_LEDS- blobSize);
    //FastLED.show(); 
  }
}



void breathingBrightness(CRGB* ledsArray, int indexController, int numLeds, int minBrightness, int maxBrightness, int speed, int _fadeAmount = 1){
  static int brightness = minBrightness;
  static int fadeAmount = 5;
  static int timer = millis();
  if (millis() - timer > speed){
    timer = millis();
    brightness = brightness + fadeAmount;
    if (brightness <= minBrightness || brightness >= maxBrightness){
      fadeAmount = -fadeAmount;
    }
    //int realBrightness = ease8InOutQuad(dim8_lin(brightness));
    fadeToBlackBy(ledsArray, numLeds, dim8_lin(brightness));
    controllers[indexController]->showLeds(255);
  }
}

void breathingBrightnessSegment(CRGB* ledsArray, int indexController, int startLed ,int numLeds, int minBrightness, int maxBrightness, int speed, int _fadeAmount = 1){
  static int brightness = minBrightness;
  static int fadeAmount = 5;
  static int lastFrame = 0;
  if (frameCount - lastFrame > speed){
    lastFrame = frameCount;
    brightness = brightness + fadeAmount;
    if (brightness <= minBrightness || brightness >= maxBrightness){
      fadeAmount = -fadeAmount;
    }
    int realBrightness = dim8_lin(brightness);
    fadeToBlackBy(&ledsArray[startLed], numLeds, 255 - realBrightness);
    //controllers[indexController]->showLeds(255);
  }
} */
#endif // SEGMENT_H