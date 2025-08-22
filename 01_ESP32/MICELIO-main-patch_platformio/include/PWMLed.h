#ifndef PWMLED_H
#define PWMLED_H

#include <Arduino.h>
#include <FastLED.h>

#define PWM_BIT_RESOLUTION 16
#define PWM_FREQUENCY 200

class PWMLed {
public:
  
  PWMLed(int dataPin, float maxBrightnessPercentage, float minBrightnessPercentage, int pwmChannel);
  int brightness = pow(2,PWM_BIT_RESOLUTION) - 1;
  void begin();
  void show();
  void breathingBrightness(int frameCount, float speed);
  void resetBrightness(int value) { brightness = value; }
  void updateBrightness(int targetBrigthness, int speed);
  void PWMbreathingBrightness(int frameCount, int minBrightness, int maxBrightness, int speed, int _fadeAmount);
  void setBrightnessPercentage(int percentage);
  int getMinBrightness() {return minBrightness;}
  int getMaxBrightness() {return maxBrightness;}
  void setMinBrightness(float minBrightnessPercentage) {calculateBrightness(minBrightnessPercentage, maxBrightness);}
  void setMaxBrightness(float maxBrightnessPercentage) {calculateBrightness(minBrightness, maxBrightnessPercentage);}
  float progress = 0;
private:
  unsigned long lastFrame = 0;
  int minBrightness;
  int maxBrightness;
  int dataPin;
  int pwmChannel;
  int intermediateBrightness;
  void calculateBrightness(float minBrightnessPercentage, float maxBrightnessPercentage);
  float easeInOutExpo(float x);
  int steps = pow(2,PWM_BIT_RESOLUTION);

};

PWMLed::PWMLed(int dataPin, float maxBrightnessPercentage, float minBrightnessPercentage, int pwmChannel) : dataPin(dataPin), pwmChannel(pwmChannel){
  float minBrightnessFloat = minBrightnessPercentage * ( pow(2,PWM_BIT_RESOLUTION) - 1);
  float maxBrightnessFloat = maxBrightnessPercentage *  (pow(2,PWM_BIT_RESOLUTION) - 1);
  minBrightness = int(minBrightnessFloat);
  maxBrightness = int(maxBrightnessFloat);

}

void PWMLed::begin() {
  ledcAttach(dataPin, PWM_FREQUENCY, PWM_BIT_RESOLUTION);
  //analogWriteFrequency(dataPin, PWM_FREQUENCY);
  //analogWriteResolution(dataPin,PWM_BIT_RESOLUTION);
}

void PWMLed::show() {
  ledcWrite(dataPin, brightness);
  //analogWrite(dataPin, brightness);
}

float PWMLed::easeInOutExpo(float x) {
  return x == 0 ? 0 : x == 1 ? 1 : x < 0.5 ? pow(2, 20 * x - 10) / 2 : (2 - pow(2, -20 * x + 10)) / 2;
}

uint16_t triwave16(uint16_t in) {
  if (in & 0x8000) {
    in = 0xFFFF - in;
  }
  return in << 1;
}

void PWMLed::breathingBrightness(int frameCount, float speed){
  //brightness = maxBrightness*sin8(frameCount * speed)/PWM_BIT_RESOLUTION + minBrightness;
  //brightness = map(sin8(frameCount * speed), 0, 255, minBrightness, maxBrightness);
  // progress = int(frameCount * speed) % steps;
  // brightness = sin16(progress) + 32767;
  // float phasor = triwave16(progress);
  // float bri = brightness/65534.0;
  // brightness = minBrightness + (easeInOutExpo(bri) * maxBrightness);

  float phasorSin = beatsin16(int(speed), 0, 65535);
  float easeInOutSin = ease16InOutQuad(phasorSin)/65535.0;
  brightness =  minBrightness + int(easeInOutSin * (maxBrightness - minBrightness));
  //Serial.print("phasorSin: "); Serial.print(phasorSin); Serial.print(" easeInOutSin: "); Serial.print(easeInOutSin); Serial.print(" brightness: "); Serial.println(brightness);

  // float easeInOut = ease16InOutQuad(phasor)/65535.0;
  // brightness =  minBrightness + int(easeInOut * (maxBrightness - minBrightness));
  //brightness = bri * (maxBrightness-minBrightness) + minBrightness;
}

void PWMLed::updateBrightness(int targetBrigthness, int speed){
  if (brightness < targetBrigthness){
    brightness += speed;
  } else if (brightness > targetBrigthness){
    brightness -= speed;
  }
}

void PWMLed::PWMbreathingBrightness(int frameCount, int minBrightness, int maxBrightness, int speed, int _fadeAmount = 1){
  static int fadeAmount = _fadeAmount;
  static unsigned long lastUpdate = 0;
  
  if (frameCount - lastUpdate > speed){
    lastUpdate = frameCount;
    intermediateBrightness = intermediateBrightness + fadeAmount;
    if (intermediateBrightness <= minBrightness || intermediateBrightness >= maxBrightness){
      fadeAmount = -fadeAmount;
    }
    int realBrightness = ease16InOutQuad(intermediateBrightness);
    brightness = realBrightness;
  }
}

void PWMLed::setBrightnessPercentage(int percentage) {
  brightness = map(percentage, 0, 100, minBrightness, maxBrightness);
}

void PWMLed::calculateBrightness(float minBrightnessPercentage, float maxBrightnessPercentage){
  float minBrightnessFloat = minBrightnessPercentage * ( pow(2,PWM_BIT_RESOLUTION) - 1);
  float maxBrightnessFloat = maxBrightnessPercentage *  (pow(2,PWM_BIT_RESOLUTION) - 1);
  minBrightness = int(minBrightnessFloat);
  maxBrightness = int(maxBrightnessFloat);
}

#endif


