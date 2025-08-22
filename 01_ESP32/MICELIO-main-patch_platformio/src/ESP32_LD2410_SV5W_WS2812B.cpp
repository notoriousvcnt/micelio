/*
 * Controlador de Luces LED WS2812b, PWM, DY-SV5W y LD2410c
  Pines:
  - DY-SV5W: RX 16, TX 17, Busy: 5
  - LD2410c: RX 32, TX 33
  - WS2812b: 13, 14, 21, 25, 26 
  - PWM: 4, 19, 22, 23, 27
  - LED para información: 24
  - RPI Zero 2 W: RX 01, TX 03
 * 
 */
#include <Arduino.h>
#include <HardwareSerial.h> 
#include <FastLED.h>
#include <ld2410.h>
#ifndef RASPI

#include <DYPlayerArduino.h>
#endif

#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <WebSerial.h>
#include <Preferences.h>

#ifdef SENSOR
#include <movingAverage.h>
#endif

#include "Segment.h"
#include <PWMLed.h>

//Global configs
#define CURRENT_LIMIT_AMPS 3000
#define GLOBAL_UPDATE_TIME_MS 16
#define SERIAL_SPEED 9600
#define CHANGE_TIME_MS 180000
#define TOTAL_DEVICES 8
//#define BUTTONS_AND_POTENTIOMETER

//State Definitions
#define STATE_NO_PRESENCE 0 // state 0: no presence detected
#define STATE_PRESENCE_DETECTED 1 // state 1: presence detected
#define STATE_STATIONARY_TARGET_DETECTED 2 // state 2: stationary target detected

//Monitor Serial Configs
#define MONITOR_SERIAL Serial
#define RADAR_SERIAL Serial1
//Radar configs
#define RADAR_RX_PIN 32
#define RADAR_TX_PIN 33
#define RADAR_UPDATE_TIME_MS 300

//Audio Configs DY-SV5W
#define AUDIO_RX_PIN 16
#define AUDIO_TX_PIN 17
#define AUDIO_BUSY_PIN 5
#define AUDIO_MODULE_MIN_LEVEL 10
#define AUDIO_MODULE_MAX_LEVEL 30

//RPI Zero configs
#define RPI_RX_PIN 1
#define RPI_TX_PIN 3
#define MIN_LEVEL_RPI 0.2
#define MAX_LEVEL_RPI 1.0
#define PREGAIN_CONFIG 2.0

//WS2812b Configs
//WS2812b: 13, 14, 21, 25, 26
#define DATA_PIN_STRIP1 13
#define DATA_PIN_STRIP2 14
#define DATA_PIN_STRIP3 21
#define DATA_PIN_STRIP4 25
#define DATA_PIN_STRIP5 26

#ifndef CAPULLO
#define MAX_NUM_LEDS 300
#endif

#ifdef CAPULLO 
#define MAX_NUM_LEDS 1200
#endif

#define NUM_STRIPS 5

#define BUILT_IN_LED 2

//PWM Leds Configs
//PWM: 4, 19, 22, 23, 27
#define MIN_BRIGHTNESS_PWM_PERCENTAGE 0
#define MAX_BRIGHTNESS_PWM_PERCENTAGE 0.1 //recomendado 0.005, se pone valor mas alto para pruebas
#define DATA_PIN_PWM1 4
#define DATA_PIN_PWM2 19
#define DATA_PIN_PWM3 22
#define DATA_PIN_PWM4 23
#define DATA_PIN_PWM5 27

//LD2410 Variables
ld2410 radar;
uint32_t lastReading = 0;
bool radarConnected = false;
unsigned long stationaryTimer = 0;
bool timerHasStarted = false;
int distanceArray[10];
uint16_t stationaryTargetDistanceThreshold = 100;
uint16_t stationarytargetEnergyThreshold = 80;
uint16_t stillTimeThresholdMs = 5000;
uint16_t radarDistance;

//LED Strip variables
//array for LED strip
CRGB ledsStrip1[MAX_NUM_LEDS];
Segment segment1(ledsStrip1, 0, MAX_NUM_LEDS, 0);

#ifdef TEST_RGB_PINS
CRGB ledsStrip2[MAX_NUM_LEDS];
CRGB ledsStrip3[MAX_NUM_LEDS];
CRGB ledsStrip4[MAX_NUM_LEDS];
CRGB ledsStrip5[MAX_NUM_LEDS];
Segment segment2(ledsStrip2, 0, MAX_NUM_LEDS, 1);
Segment segment3(ledsStrip3, 0, MAX_NUM_LEDS, 2);
Segment segment4(ledsStrip4, 0, MAX_NUM_LEDS, 3);
Segment segment5(ledsStrip5, 0, MAX_NUM_LEDS, 4);
#endif

CRGB gradientArray[100];
CHSV color1 = CHSV(147,165,242);
CHSV color2 = CHSV(193,165,242);
CHSV color = CHSV(128, 128, 128);
CLEDController *controllers[NUM_STRIPS];
unsigned long globalTimer;
unsigned long counter;
bool lightsChangeFlag = true;

//PWM LED configs
PWMLed pwmLed1(DATA_PIN_PWM1, MAX_BRIGHTNESS_PWM_PERCENTAGE, MIN_BRIGHTNESS_PWM_PERCENTAGE, 1);
#ifdef TEST_PWM_PINS
PWMLed pwmLed2(DATA_PIN_PWM2, MAX_BRIGHTNESS_PWM_PERCENTAGE, MIN_BRIGHTNESS_PWM_PERCENTAGE, 2);
PWMLed pwmLed3(DATA_PIN_PWM3, MAX_BRIGHTNESS_PWM_PERCENTAGE, MIN_BRIGHTNESS_PWM_PERCENTAGE, 3);
PWMLed pwmLed4(DATA_PIN_PWM4, MAX_BRIGHTNESS_PWM_PERCENTAGE, MIN_BRIGHTNESS_PWM_PERCENTAGE, 4);
PWMLed pwmLed5(DATA_PIN_PWM5, MAX_BRIGHTNESS_PWM_PERCENTAGE, MIN_BRIGHTNESS_PWM_PERCENTAGE, 5);
#endif

// DYPlayer Variables
#ifndef RASPI
HardwareSerial AUDIO_SERIAL(2); // Example initialization
DY::Player player(&AUDIO_SERIAL);
bool playFlag = false;
bool playerBusyStatus;
bool lastBusyStatus = LOW;
bool stateZeroFlag = true;
bool queryFlag = true;
#endif
uint8_t lowVolume = 10;
uint8_t highVolume = 15;

/* RPI Variables */
unsigned long RPI_lastFrame;
float RPI_lowVolume = 0.2;
float RPI_highVolume = 1.0;

/* other variables */
// //Button Configs
// int buttonPins[4] = {27,22,23,34};
// int previousButtonsState[4];
// int buttons[4];
// int buttonsSwitch[4] = {1,1,1,1};

bool ledStatus = false;


/*GLOBAL CONFIGS*/
bool state = STATE_NO_PRESENCE;
bool previousState = STATE_NO_PRESENCE;
bool changeState = LOW;
unsigned long frameCount;
unsigned long lastFrame = 0;
unsigned long globalRotativaTimer = 0; 
uint8_t RotativaCounter = 0;
bool changeFlag = true;
bool startFlag = false;
bool changedByWebSerial = false;
int lightsChangeState = 0;
bool resetRGB = false;\

/*WiFi Configs*/
const char *ssid = "SSID";
const char *password = "PASSWORD";


/* Preferences configs */
Preferences preferences;
struct DeviceSettings
{
  uint8_t lowVolume;
  uint8_t highVolume;
  float RPI_lowVolume;
  float RPI_highVolume;
  float PWMminBrightness;
  float PWMmaxBrightness;
  float PWMspeed;
  float RGBbrightness;
  float RGBspeed;
  float RPI_PreGain;
  uint8_t RGBfadePercent;
  uint8_t blobSize;
  uint8_t deviceNumber;
  uint32_t rotativaTimeSeconds;
  String deviceName;
  String device_IP;
} deviceSettings;


// IP Addresses
IPAddress local_IP;
IPAddress gateway;
IPAddress subnet;

bool ledState = LOW;

AsyncWebServer server(80);

/* FUNCTIONS DECLARATIONS*/

void readRadar();
void checkAndDisplayBusyPin();
void updateLights();
void updateBuiltinLed();
void handleRGBstrips();
void handlePWMstrips();
void globalUpdate();
void WiFiSetup();
void recvMsg(uint8_t *data, size_t len);
void sendDataToRASPI();
void globalUpdateNoInteractive();
void SerialRecvMsg();
void sendDataToAudioModule();
void loadPreferences();
void savePreferences();
void uploadDeviceProperties();
String getPreferenceValue(String key);
void checkIfPWMConfigsAreCorrect();

#ifdef DEPRECATED_CODE
#ifdef BUTTONS_AND_POTENTIOMETER
  void setupButtons();
  void readButtons();
  void readInputs();
#endif
#ifdef RASPI
  void readAndUpdatePureDataStatus();
#endif
void readAndUpdatePlayerStatus();
#endif

/* FUNCTIONS DEFINITIONS*/
void setup(){
  #ifdef BUTTONS_AND_POTENTIOMETER
    setupButtons();
  #endif
  
  #ifdef SENSOR
  radar.debug(MONITOR_SERIAL); //Uncomment to show debug information from the library on the Serial Monitor. By default this does not show sensor reads as they are very frequent.
  RADAR_SERIAL.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN); //UART for monitoring the radar
  #endif

  #ifndef RASPI
  MONITOR_SERIAL.begin(115200); //Feedback over Serial Monitor
  AUDIO_SERIAL.begin(9600, SERIAL_8N1, AUDIO_RX_PIN, AUDIO_TX_PIN); //UART for monitoring the audio module
  #endif

  #ifdef RASPI
  MONITOR_SERIAL.begin(115200); //Feedback over Serial Monitor
  #endif

  //WiFi Configs
  pinMode(2,OUTPUT);
  loadPreferences();
  #ifdef WIFI
    WiFiSetup();
    #ifdef UPLOAD_DEVICE_PROPERTIES
      uploadDeviceProperties();
      savePreferences();
    #endif
    delay(500);
  #endif

  //radar initialization
  #ifdef SENSOR	
  MONITOR_SERIAL.print(F("\nConnect LD2410 radar TX to GPIO:"));
  MONITOR_SERIAL.println(RADAR_RX_PIN);
  MONITOR_SERIAL.print(F("Connect LD2410 radar RX to GPIO:"));
  MONITOR_SERIAL.println(RADAR_TX_PIN);
  MONITOR_SERIAL.print(F("LD2410 radar sensor initialising: "));
  

  if(radar.begin(RADAR_SERIAL))
  {
    MONITOR_SERIAL.println(F("OK"));
    radarConnected = true;
    #ifdef WEBSERIAL
    WebSerial.println("LD2410C connected");
    #endif
    MONITOR_SERIAL.print(F("LD2410 firmware version: "));
    MONITOR_SERIAL.print(radar.firmware_major_version);
    MONITOR_SERIAL.print('.');
    MONITOR_SERIAL.print(radar.firmware_minor_version);
    MONITOR_SERIAL.print('.');
    MONITOR_SERIAL.println(radar.firmware_bugfix_version, HEX);
    radar.turnOnBluetooth();
  } else {
    MONITOR_SERIAL.println(F("LD2410C not connected"));
    radarConnected = false;
    #ifdef WEBSERIAL
    WebSerial.println("LD2410C not connected");
    #endif
    //ESP.restart();
  }
  #endif

  //player initialization
  #ifndef RASPI
  player.begin();
  player.setVolume(30); // 0-30
  #endif

  #ifdef WS2812b
  //LED Strip initialization
  controllers[0] = &FastLED.addLeds<WS2812B, DATA_PIN_STRIP1, GRB>(ledsStrip1, MAX_NUM_LEDS);
  segment1.setColor(CRGB::Orchid);
  #ifdef TEST_RGB_PINS
  controllers[1] = &FastLED.addLeds<WS2812B, DATA_PIN_STRIP2, GRB>(ledsStrip2, MAX_NUM_LEDS);
  controllers[2] = &FastLED.addLeds<WS2812B, DATA_PIN_STRIP3, GRB>(ledsStrip3, MAX_NUM_LEDS);
  controllers[3] = &FastLED.addLeds<WS2812B, DATA_PIN_STRIP4, GRB>(ledsStrip4, MAX_NUM_LEDS);
  controllers[4] = &FastLED.addLeds<WS2812B, DATA_PIN_STRIP5, GRB>(ledsStrip5, MAX_NUM_LEDS);
  segment2.setColor(CRGB::RoyalBlue);
  #endif

  FastLED.setMaxPowerInVoltsAndMilliamps(5,CURRENT_LIMIT_AMPS);
  FastLED.delay(1000);
  #endif

  #ifdef PWMLeds
  //PWM LED initialization
  pwmLed1.setMinBrightness(deviceSettings.PWMminBrightness);
  pwmLed1.setMaxBrightness(deviceSettings.PWMmaxBrightness);

  pwmLed1.begin();

  #ifdef TEST_PWM_PINS
  pwmLed2.setMinBrightness(deviceSettings.PWMminBrightness);
  pwmLed2.setMaxBrightness(deviceSettings.PWMmaxBrightness);

  pwmLed3.setMinBrightness(deviceSettings.PWMminBrightness);
  pwmLed3.setMaxBrightness(deviceSettings.PWMmaxBrightness);

  pwmLed4.setMinBrightness(deviceSettings.PWMminBrightness);
  pwmLed4.setMaxBrightness(deviceSettings.PWMmaxBrightness);

  pwmLed5.setMinBrightness(deviceSettings.PWMminBrightness);
  pwmLed5.setMaxBrightness(deviceSettings.PWMmaxBrightness);

  pwmLed2.begin();
  pwmLed3.begin();
  pwmLed4.begin();
  pwmLed5.begin();
  #endif
  #endif
  
  checkIfPWMConfigsAreCorrect();
  //Builtin LED
  pinMode(2, OUTPUT); 

  #ifdef DEBUG_SERIAL 
  MONITOR_SERIAL.println(F("Setup Complete"));
  #endif

  #ifndef RASPI
  player.setCycleMode(DY::Random);
  delay(500);
  player.play();
  #endif
}


void loop()
{
  #ifdef WIFI
  ArduinoOTA.handle();
  #endif

  #ifndef SENSOR
  globalUpdateNoInteractive();
  #endif
  //read radar data
  #ifdef SENSOR
  readRadar();
  #endif

  #ifdef RASPI
  sendDataToRASPI();
  SerialRecvMsg();
  #endif

  #ifndef RASPI
  sendDataToAudioModule();
  #endif

  //update light
  updateLights();

  //update Built-In led, visual feedback for radar detection or other things
  updateBuiltinLed();
}

void checkIfPWMConfigsAreCorrect(){
  if (pow(2, PWM_BIT_RESOLUTION) * PWM_FREQUENCY > 40 * pow(10,6)){
    MONITOR_SERIAL.println("PWM Frequency and Resolution are too high, please check the values");
    ESP.restart();
  }
}

void uploadDeviceProperties(){
  preferences.begin("deviceSettings", false);
  preferences.putString("deviceName", DEVICE_NAME);
  preferences.putString("device_IP", DEVICE_IP);
  preferences.putInt("deviceNumber", DEVICE_NUMBER);
  preferences.end();
}

void loadPreferences(){
  preferences.begin("deviceSettings", false);
  deviceSettings.deviceName = preferences.getString("deviceName", "default");
  deviceSettings.device_IP = preferences.getString("device_IP");
  deviceSettings.deviceNumber = preferences.getInt("deviceNumber", 0);
  deviceSettings.lowVolume = preferences.getInt("lowVolume", 10);
  deviceSettings.highVolume = preferences.getInt("highVolume", 15);
  deviceSettings.RPI_lowVolume = preferences.getFloat("RPI_lowVolume", 0.2);
  deviceSettings.RPI_highVolume = preferences.getFloat("RPI_highVolume", 1.0);
  deviceSettings.PWMminBrightness = preferences.getFloat("PWMminBri", 0.0);
  deviceSettings.PWMmaxBrightness = preferences.getFloat("PWMmaxBri", 0.004);
  deviceSettings.PWMspeed = preferences.getFloat("PWMspeed", 1);
  deviceSettings.RGBbrightness = preferences.getFloat("RGBbri", 0.1);
  deviceSettings.RGBspeed = preferences.getFloat("RGBspeed", 4);
  deviceSettings.RPI_PreGain = preferences.getFloat("RPI_PreGain", 2.0);
  deviceSettings.RGBfadePercent = preferences.getInt("RGBfadePercent", 50);
  deviceSettings.blobSize = preferences.getInt("blobSize", 14);
  #ifdef CAPULLO
  deviceSettings.rotativaTimeSeconds = preferences.getInt("rotativaTimeSec", 360);
  #endif
  deviceSettings.rotativaTimeSeconds = preferences.getInt("rotativaTimeSec", 180);
  preferences.end();
}

void savePreferences(){
  preferences.begin("deviceSettings", false);
  preferences.putString("deviceName", deviceSettings.deviceName);
  preferences.putString("device_IP", deviceSettings.device_IP);
  preferences.putInt("deviceNumber", deviceSettings.deviceNumber);
  preferences.putInt("lowVolume", deviceSettings.lowVolume);
  preferences.putInt("highVolume", deviceSettings.highVolume);
  preferences.putFloat("RPI_lowVolume", deviceSettings.RPI_lowVolume);
  preferences.putFloat("RPI_highVolume", deviceSettings.RPI_highVolume);
  preferences.putFloat("PWMminBri", deviceSettings.PWMminBrightness);
  preferences.putFloat("PWMmaxBri", deviceSettings.PWMmaxBrightness);
  preferences.putFloat("PWMspeed", deviceSettings.PWMspeed);
  preferences.putFloat("RGBbri", deviceSettings.RGBbrightness);
  preferences.putFloat("RGBspeed", deviceSettings.RGBspeed);
  preferences.putFloat("RPI_PreGain", deviceSettings.RPI_PreGain);
  preferences.putInt("RGBfadePercent", deviceSettings.RGBfadePercent);
  preferences.putInt("blobSize", deviceSettings.blobSize);
  preferences.putInt("rotativaTimeSec", deviceSettings.rotativaTimeSeconds);
  preferences.end();
  Serial.println("Preferences saved.");
}

void WiFiSetup(){
  local_IP.fromString(deviceSettings.device_IP);
  gateway.fromString(GATEWAY_IP);
  subnet.fromString(SUBNET_IP);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println(F("STA Failed to configure"));
  }
  
  WiFi.setHostname(deviceSettings.deviceName.c_str());

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println(WiFi.getHostname());

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();

  Serial.println(F("Ready"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  digitalWrite(2,HIGH);

  // Configura una página web básica
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "¡Servidor ESP32 funcionando!");
  });
  
  #ifdef WEBSERIAL
  WebSerial.msgCallback(recvMsg);
  WebSerial.begin(&server);
  #endif

  server.begin();
}

#ifdef SENSOR
void readRadar(){
  radar.read();
  if(radar.isConnected() && millis() -  lastReading > RADAR_UPDATE_TIME_MS)  //Report every 500ms
  {
    lastReading = millis();
    previousState = state;

    radarDistance = radar.detectionTargetDistance();
    updateDistanceArray(distanceArray, radarDistance);

    #ifdef DEBUG_SERIAL
    MONITOR_SERIAL.println("--------------------------");
    MONITOR_SERIAL.print("state: "); MONITOR_SERIAL.println(state);
    MONITOR_SERIAL.print("Distance: "); 
    MONITOR_SERIAL.println(radarDistance);
    #endif

    if (radar.presenceDetected()){
      #ifdef DEBUG_SERIAL
      MONITOR_SERIAL.println("Presence Detected");
      #endif
      //monitor radar data
      #ifdef DEBUG_SERIAL
      MONITOR_SERIAL.print("Moving Energy: "); MONITOR_SERIAL.println(radar.movingTargetEnergy());
      MONITOR_SERIAL.print("Stationary Energy: "); MONITOR_SERIAL.println(radar.stationaryTargetEnergy());
      MONITOR_SERIAL.print("Detection Distance: "); MONITOR_SERIAL.println(radar.detectionTargetDistance());
      #endif
      
      //if person stays still for 15 seconds, change to state 2
      if (radar.stationaryTargetEnergy() >= stationarytargetEnergyThreshold && radar.detectionTargetDistance() <= stationaryTargetDistanceThreshold){
        //start timer
        if (!timerHasStarted){
          stationaryTimer = millis();
          timerHasStarted = true;
        }
        //check if timer has reached 15 seconds
        if (millis() - stationaryTimer >=  stillTimeThresholdMs){
          #ifdef DEBUG_SERIAL
          MONITOR_SERIAL.print("Stationary Target Detected for longer than ");
          MONITOR_SERIAL.print(stillTimeThresholdMs); MONITOR_SERIAL.println(" ms");
          #endif
          state = 2;
        } else { 
          #ifdef DEBUG_SERIAL
          MONITOR_SERIAL.println("Stationary Target Detected");
          #endif
          state = 1;
        }
      } else { //if detected but not stationary, change to state 1
        state = 1;
        timerHasStarted = false;
      }
        
    } else { //if not presence detected, change to state 0
      #ifdef DEBUG_SERIAL
      MONITOR_SERIAL.println("No Presence Detected");
      #endif
      state = 0;
    }
  }

  if (state != previousState){
    changeState = HIGH;
  } else {
    changeState = LOW;
  }

}
#endif

#ifndef RASPI
void checkAndDisplayBusyPin(){
  lastBusyStatus = playerBusyStatus;
  playerBusyStatus = digitalRead(AUDIO_BUSY_PIN);

  if (player.checkPlayState() == DY::PlayState::Stopped && queryFlag){
    playerBusyStatus = LOW;
    queryFlag = false;
    MONITOR_SERIAL.println("Player is not busy");
  }

  //digitalWrite(BUILT_IN_LED, playerBusyStatus);
}
#endif




void updateLights(){
  if (state > 0){
    handleRGBstrips();
    resetRGB = false;
  } else if (state == 0) {
    fadeToBlackBy(ledsStrip1, MAX_NUM_LEDS, 10);
    #ifdef TEST_RGB_PINS
    fill_solid(ledsStrip2, MAX_NUM_LEDS, CRGB::Black);
    fill_solid(ledsStrip3, MAX_NUM_LEDS, CRGB::Black);
    fill_solid(ledsStrip4, MAX_NUM_LEDS, CRGB::Black);
    fill_solid(ledsStrip5, MAX_NUM_LEDS, CRGB::Black);

    segment2.resetPulsePosition();
    segment3.resetPulsePosition();
    segment4.resetPulsePosition();
    segment5.resetPulsePosition();
    #endif
  }
  #ifdef PWMLeds
  handlePWMstrips();
  #endif

  if (millis() - globalTimer > GLOBAL_UPDATE_TIME_MS ){
    frameCount++;
    globalTimer = millis();
  }

   #ifdef PWMLeds  
    pwmLed1.show();
    #ifdef TEST_PWM_PINS
    pwmLed2.show();
    pwmLed3.show();
    pwmLed4.show();
    pwmLed5.show();
    #endif
    #endif

  #ifdef WS2812b
  controllers[0]->showLeds(segment1.brightness);
  #ifdef TEST_RGB_PINS
  controllers[1]->showLeds(segment2.brightness);
  controllers[2]->showLeds(segment3.brightness);
  controllers[3]->showLeds(segment4.brightness);
  controllers[4]->showLeds(segment5.brightness);
  #endif
  #endif
} 

void updateBuiltinLed(){
  if (state)
  {
    ledStatus = HIGH;
  } else {
    ledStatus = LOW;
  }

  digitalWrite(BUILT_IN_LED, ledStatus);
}

#ifdef SENSOR
void globalUpdate(){
 switch(state){
    case 0:
      if (stateZeroFlag){
        //fill_solid(ledsStrip1, MAX_NUM_LEDS, CRGB::Black);
        segment1.setColor(CRGB::Black);
        segment1.resetPulsePosition();
        stateZeroFlag = false;
        Serial.println("State 0");
      }
      break;
    case 1:
      stateZeroFlag = true;
      segment1.resetBrightness(255);

      segment1.pulseAnimation(frameCount, 3, 1, CRGB::Red, 75);
      //PWM LED Handler
      //PWMbreathingBrightness(0, 30000, 10, 200);
      break;
    case 2:
      stateZeroFlag = true;
      segment1.resetBrightness(0);
      segment1.resetPulsePosition();
      //fill_solid(ledsStrip1, MAX_NUM_LEDS, CRGB::DarkOrchid);
      segment1.setColor(CRGB::DarkOrchid);
      segment1.breathingBrightness(frameCount,0 , 100, 5, 5);
      //PWM LED Handler
      //PWMbreathingBrightness(0, 30000, 10, 200);
      break;
  }
}
#endif

void globalUpdateNoInteractive(){

  
  if (changeFlag){
    #ifdef WEBSERIAL
    WebSerial.print("State: "); WebSerial.println(state); WebSerial.print("changeFlag: "); WebSerial.println(changeFlag);
    #endif
    Serial.print("State: "); Serial.println(state); Serial.print("changeFlag: "); Serial.println(changeFlag);
  }
 


  if (!changedByWebSerial){
    if (millis() - globalRotativaTimer > deviceSettings.rotativaTimeSeconds * 1000){
      globalRotativaTimer = millis();
      RotativaCounter = (RotativaCounter + 1) % TOTAL_DEVICES;
      Serial.print("Rotativa Counter: "); Serial.println(RotativaCounter);
      if (RotativaCounter == deviceSettings.deviceNumber){
        state = 1;
        resetRGB = true;
      } else {
        state = 0;
      }
      changeFlag = true;
      lightsChangeFlag = true;
    } else {
        changeFlag = false;
    }
  } else {
    changedByWebSerial = false;
  }

  if (state > 0){
      digitalWrite(2,HIGH);
    } else {
      digitalWrite(2,LOW);
    }
}

void handleRGBstrips(){
  //Serial.print("ChangFlag on handleRGBstrips: "); Serial.println(changeFlag);
  #ifdef WS2812b
  //segment1.pulseAnimation(frameCount, deviceSettings.RGBspeed, deviceSettings.blobSize, color, deviceSettings.RGBfadePercent);
  segment1.pulseAnimation2(deviceSettings.RGBspeed, deviceSettings.blobSize, color, deviceSettings.RGBfadePercent, resetRGB);
  
  
  #ifdef TEST_RGB_PINS
  segment2.pulseAnimation2(deviceSettings.RGBspeed, deviceSettings.blobSize, color, deviceSettings.RGBfadePercent, changeFlag);
  segment3.pulseAnimation2(deviceSettings.RGBspeed, deviceSettings.blobSize, color, deviceSettings.RGBfadePercent, changeFlag);
  segment4.pulseAnimation2(deviceSettings.RGBspeed, deviceSettings.blobSize, color, deviceSettings.RGBfadePercent, changeFlag);
  segment5.pulseAnimation2(deviceSettings.RGBspeed, deviceSettings.blobSize, color, deviceSettings.RGBfadePercent, changeFlag);
  #endif
  #endif
}

void handlePWMstrips(){
  #ifdef PWMLeds
  pwmLed1.breathingBrightness(frameCount, deviceSettings.PWMspeed);
  #ifdef TEST_PWM_PINS
  pwmLed2.breathingBrightness(frameCount, deviceSettings.PWMspeed);
  pwmLed3.breathingBrightness(frameCount, deviceSettings.PWMspeed);
  pwmLed4.breathingBrightness(frameCount, deviceSettings.PWMspeed);
  pwmLed5.breathingBrightness(frameCount, deviceSettings.PWMspeed);
  #endif
  #endif
}


void updateMinMaxBriPWM(float minBri, float maxBri){
  #ifdef PWMLeds
  pwmLed1.setMinBrightness(minBri);
  pwmLed1.setMaxBrightness(maxBri);

  #ifdef TEST_PWM_PINS
  pwmLed2.setMinBrightness(minBri);
  pwmLed2.setMaxBrightness(maxBri);

  pwmLed3.setMinBrightness(minBri);
  pwmLed3.setMaxBrightness(maxBri);

  pwmLed4.setMinBrightness(minBri);
  pwmLed4.setMaxBrightness(maxBri);

  pwmLed5.setMinBrightness(minBri);
  pwmLed5.setMaxBrightness(maxBri);
  #endif
  #endif
}

/* WebSerial */
#ifdef WEBSERIAL
void recvMsg(uint8_t *data, size_t len) {
  WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++) {
    d += char(data[i]);
  }
  WebSerial.println(d);

  if (d == "status") {
    WebSerial.print("ESP32 Name is: "); WebSerial.println(deviceSettings.deviceName);
    WebSerial.print("Radar is "); WebSerial.println(radarConnected ? "Connected" : "Not Connected");
    WebSerial.println("Sending Status...");
    WebSerial.print("Distance "); WebSerial.println(radarDistance);
    WebSerial.print("State "); WebSerial.println(state);
    #ifndef RASPI
    WebSerial.print("Player Busy Status: "); WebSerial.println(playerBusyStatus);
    #endif
  } else if (d == "save"){
    savePreferences();
    WebSerial.println("Preferences Saved");
    loadPreferences();
  } else {
    int spaceIndex = d.indexOf(' ');
    if (spaceIndex != -1) {
      String command = d.substring(0, spaceIndex);
      String valueStr = d.substring(spaceIndex + 1);
      Serial.print("Command value: "); Serial.println(valueStr);
      float value = valueStr.toFloat();
      if (command == "setMinBrightness") {
        // Assuming you have a global brightness variable
          deviceSettings.PWMminBrightness = value;
          updateMinMaxBriPWM(deviceSettings.PWMminBrightness, deviceSettings.PWMmaxBrightness);
        WebSerial.print("minBrightness set to: "); WebSerial.println(value);
      } else if (command == "setMaxBrightness") {
        // Assuming you have a global brightness variable
          deviceSettings.PWMmaxBrightness = value;
          updateMinMaxBriPWM(deviceSettings.PWMminBrightness, deviceSettings.PWMmaxBrightness);
        WebSerial.print("maxBrightness set to: "); WebSerial.println(value);
      } else if (command == "setRGBBrightness") {
        // Assuming you have a global brightness variable
        // brightness = (int)value;
        WebSerial.print("maxBrightness set to: "); WebSerial.println(value);
      } else if (command == "setRGBspeed") {
        // Assuming you have a global speed variable
        deviceSettings.RGBspeed = value;
        WebSerial.print("Speed set to: "); WebSerial.println(value);
      } else if (command == "setRGBfadePercent"){
        deviceSettings.RGBfadePercent = value;
        WebSerial.print("Fade Percent set to: "); WebSerial.println(value);
      } else if (command == "setBlobSize"){
        deviceSettings.blobSize = value;
        WebSerial.print("Blob Size set to: "); WebSerial.println(value);
      } else if (command == "setState") {
        state = value;
        changeFlag = true;
        lightsChangeFlag = true;
        changedByWebSerial = true;
        resetRGB = true;
        WebSerial.print("State set to: "); WebSerial.println(value);
      } else if (command == "setRotativaTime"){
        deviceSettings.rotativaTimeSeconds = value;
        WebSerial.print("Rotativa Time set to: "); WebSerial.println(value);
      }
      #ifndef RASPI
      else if (command == "setVolume"){
        player.setVolume((int)value);
        WebSerial.print("Volume set to: "); WebSerial.println(value);
      } else if (command == "setLowVolume") {
        lowVolume = (int)value;
        if (state == 0) {player.setVolume(lowVolume);}
        WebSerial.print("Low Volume set to: "); WebSerial.println(value);
      } else if (command == "setHighVolume") {
        highVolume = (int)value;
        if (state > 0) {player.setVolume(highVolume);}
        WebSerial.print("High volume set to: "); WebSerial.println(value);
      } 
      #endif 
      #ifdef RASPI
      else if (command == "RPIsetState") {
        state = value;
        changeFlag = true;
        lightsChangeFlag = true;
        WebSerial.print("State set to: "); WebSerial.println(value);
      }
      else if (command == "RPIsetVolume") {
        MONITOR_SERIAL.print("Level "); MONITOR_SERIAL.println(value);
        WebSerial.print("RPI Low Volume set to: "); WebSerial.println(value);
      } else if (command == "RPIsetLowVolume") {
        RPI_lowVolume = value;
        if (state == 0) { MONITOR_SERIAL.print("Level "); MONITOR_SERIAL.println(value);}
        WebSerial.print("RPI Low Volume set to: "); WebSerial.println(value);
      } else if (command == "RPIsetHighVolume") {
        RPI_highVolume = value;
        if (state > 0) { MONITOR_SERIAL.print("Level "); MONITOR_SERIAL.println(value);}
        WebSerial.print("RPI High Volume set to: "); WebSerial.println(value);
      } else if (command == "PreGain") {
        Serial.print("PreGain "); Serial.println(value);
      } 
      #endif 
      else if (command == "setPWMspeed") {
        // Assuming you have a global speed variable
        deviceSettings.PWMspeed = value;
        WebSerial.print("Speed set to: "); WebSerial.println(value);
      } else if (command == "getParam"){
        String paramValue = getPreferenceValue(valueStr);
        WebSerial.print("Param "); WebSerial.print(valueStr); WebSerial.print(" is "); WebSerial.println(paramValue);
      } else {
        WebSerial.println("Unknown command");
      }
    } else {
      WebSerial.println("Invalid format. Use 'command value'");
    }
  }
}
#endif

String getPreferenceValue(String key){
  if (key == "deviceName"){
    return deviceSettings.deviceName;
  } else if (key == "device_IP"){
    return deviceSettings.device_IP;
  } else if (key == "deviceNumber"){
    return String(deviceSettings.deviceNumber);
  } else if (key == "lowVolume"){
    return String(deviceSettings.lowVolume);
  } else if (key == "highVolume"){
    return String(deviceSettings.highVolume);
  } else if (key == "RPI_lowVolume"){
    return String(deviceSettings.RPI_lowVolume);
  } else if (key == "RPI_highVolume"){
    return String(deviceSettings.RPI_highVolume);
  } else if (key == "PWMminBri"){
    return String(deviceSettings.PWMminBrightness);
  } else if (key == "PWMmaxBri"){
    return String(deviceSettings.PWMmaxBrightness);
  } else if (key == "PWMspeed"){
    return String(deviceSettings.PWMspeed);
  } else if (key == "RGBbri"){
    return String(deviceSettings.RGBbrightness);
  } else if (key == "RGBspeed"){
    return String(deviceSettings.RGBspeed);
  } else if (key == "RPI_PreGain"){
    return String(deviceSettings.RPI_PreGain);
  } else if (key == "RGBfadePercent"){
    return String(deviceSettings.RGBfadePercent);
  } else if (key == "blobSize"){
    return String(deviceSettings.blobSize);
  } else {
    return "Invalid Key";
  }
}

void sendDataToRASPI(){
  float level;
  if (millis() - RPI_lastFrame > 1000){
    RPI_lastFrame = millis();
    if (state > 0){
      state = 1;
      level = RPI_highVolume;
    } else {
      state = 0;
      level = RPI_lowVolume;
    }
    MONITOR_SERIAL.print("State "); MONITOR_SERIAL.println(state);
    MONITOR_SERIAL.print("Level "); MONITOR_SERIAL.println(level);
  } 
}

void SerialRecvMsg(){
 if (Serial.available() > 0){
    //Serial.println("Received Data...");
    String d = Serial.readStringUntil('\n'); // Leer hasta encontrar un salto de línea (\n)
    d.trim(); // Elimina espacios en blanco al inicio o al final
    
    if (d == "s"){
      Serial.print("Start "); Serial.println(1);
      Serial.print("PreGain "); Serial.println(PREGAIN_CONFIG);
      startFlag = true;
    }
  }
}

#ifndef RASPI
void sendDataToAudioModule() {
  if (changeFlag){
    if (state > 0){
      player.setVolume(highVolume);
    } else {
      player.setVolume(lowVolume);
    }
  }
}
#endif

#ifdef DEPRECATED_CODE
/* Deprecated code */
void readAndUpdatePlayerStatus(){
  static uint32_t lastBusyUpdate = 0;
  static int nextTrack = 1;
  static int stopFlag = LOW;
  //static int playFlag = LOW;
   
  if (millis() - lastBusyUpdate >= 1500){
    lastBusyUpdate = millis();
    checkAndDisplayBusyPin();
    #ifdef DEBUG_SERIAL
    MONITOR_SERIAL.print("Player Busy Status: "); MONITOR_SERIAL.println(playerBusyStatus);
    #endif
    // if player is 0, is busy, so stop the player if also is not detected nothing
    if (!playerBusyStatus && state == 0 && !stopFlag){
      player.stop();
      stopFlag = HIGH;
      playFlag = LOW;
      #ifdef DEBUG_SERIAL
      MONITOR_SERIAL.println("Stopping Player");
      #endif
    }
    /*if playerBusyStatus is HIGH, player is not busy, so play random track using player.playSpecified(). Make random using random8(5) */
    if (playerBusyStatus && state > 0){
      //loop nextrack between 1 and 5 in order
      nextTrack = random8(91);
      player.playSpecified(nextTrack+1);
      #ifdef DEBUG_SERIAL
      MONITOR_SERIAL.print("Playing ");
      MONITOR_SERIAL.println(nextTrack);
      #endif
      #ifdef WEBSERIAL
      WebSerial.print("Playing ");
      WebSerial.println(nextTrack);
      #endif
      stopFlag = LOW;
      playFlag = HIGH;
      queryFlag = true;
    }
  }
}
#ifdef BUTTONS_AND_POTENTIOMETER
void setupButtons(){
  for (int i = 0; i < 4; i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}
void readButtons(){
  for (int i = 0; i < 3; i++){
    previousButtonsState[i] = buttons[i];
    buttons[i] = digitalRead(buttonPins[i]);
    
    //check if button was pressed, if so, toggle the switch, only print once
    //when the high to low transition is detected
    if (buttons[i] == LOW && previousButtonsState[i] == HIGH){
      buttonsSwitch[i] = !buttonsSwitch[i];
    }
    
  }
}
void readInputs(){
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate >= 150) {
    lastUpdate = millis();
    readButtons();
   
    if (!buttons[0] && previousButtonsState[0]){
      int nextNumber = random8(5);
      player.playSpecified(nextNumber);
      //MONITOR_SERIAL.print("Playing ");
      //MONITOR_SERIAL.println(nextNumber);
    }

  //   if (!buttons[0] && previousButtonsState[0]){
  //     pwmDutyCycle = (pwmDutyCycle + sumButtons) % 1024;
  //     Serial.println(pwmDutyCycle);
  //   }

  //   if (!buttons[1] && previousButtonsState[1]){
  //     pwmDutyCycle = pwmDutyCycle > 0 ? (pwmDutyCycle - sumButtons) % 1024 : 0;
  //     Serial.println(pwmDutyCycle);
  //   }
  }
}
#endif
#ifdef RASPI
void readAndUpdatePureDataStatus(){
  //as PureData is self aware of his status play, it is not necessary to check the player status, and only send data related to the LD2410c sensor.
  static uint32_t lastBusyUpdate = 0;

  if (millis() - lastBusyUpdate >= 1500 && changeState){
    lastBusyUpdate = millis();

    MONITOR_SERIAL.print("Distance "); MONITOR_SERIAL.println(radarDistance);
    MONITOR_SERIAL.print("State "); MONITOR_SERIAL.println(state);
    checkAndDisplayBusyPin();
    #ifdef DEBUG_SERIAL
    MONITOR_SERIAL.print("Player Busy Status: "); MONITOR_SERIAL.println(playerBusyStatus);
    #endif
  }
}
#endif
#endif