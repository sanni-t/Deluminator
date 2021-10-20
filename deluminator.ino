/**
   BasicHTTPSClient.ino

    Created on: 14.10.2018

*/

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Adafruit_NeoPixel.h>

#include <WiFiMulti.h>

#include <HTTPClient.h>

#include <WiFiClientSecure.h>
#include "deluminator.h"

// Create RoomLights object & initialize with appropriate LightInfo defined in
// deluminator.h
RoomLights hall_lights(HALL_LIGHTS, 3);
RoomLights room2_lights(ROOM2_LIGHTS, 1);
RoomLights *nearer_lights;

BLEScan* pBLEScan;
WiFiMulti WiFiMulti;
Adafruit_NeoPixel strip(LED_COUNT, LIGHT_PIN, NEO_GRBW + NEO_KHZ800);

// Not sure if WiFiClientSecure checks the validity date of the certificate. 
// Setting clock just to be sure...
void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {

      Serial.printf("Name: %s \t", advertisedDevice.getName().c_str());
      Serial.printf("Address: %s \t", advertisedDevice.getAddress().toString().c_str());
      Serial.printf("RSSI: %i \n", advertisedDevice.getRSSI());
      BLEAddress address = advertisedDevice.getAddress();
      
      if (address == BLEAddress(ROOM_1_BLE))
      {
        room1_rssi = advertisedDevice.getRSSI();
        rooms_found++;
      }
      else if (address == BLEAddress(ROOM_2_BLE))
      {
        room2_rssi = advertisedDevice.getRSSI();
        rooms_found++;
      }
      nearer_lights = &((room1_rssi >= room2_rssi) ? hall_lights : room2_lights);
      if (rooms_found == TOTAL_ROOMS)
      {
        Serial.printf("Stopping scan......");
        advertisedDevice.getScan()->stop();
      }
    }
};

void setupBLEScan()
{
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void setup() {

  Serial.begin(115200);

  strip.begin();           
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(150); // Set BRIGHTNESS to about 1/5 (max = 255)
  blink_lights(strip.Color(127, 127, 127, 0));

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  connect_to_wifi(); 
  setClock();
  blink_lights(strip.Color(153, 0, 255, 0));
}

bool connect_to_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SSID, PASS);

  // wait for WiFi connection
  Serial.print("Waiting for WiFi to connect...");
  while ((WiFiMulti.run() != WL_CONNECTED)) {
    // delay(5);
    Serial.print(".");
  }
  Serial.println(" connected");
  return true;
}


bool button_pressed()
{
  if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED_LEVEL && PREV_BUTTON_STATE == !BUTTON_PRESSED_LEVEL)
  {
    delay(250);
    if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED_LEVEL)
    {
      PREV_BUTTON_STATE = BUTTON_PRESSED_LEVEL;
      delay(1000);
      if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED_LEVEL)
      {
        press_kind = Button::LONG_PRESS;
      }
      else
      {
        press_kind = Button::SHORT_PRESS;
      }
      return true;
    }
  }
  else
  {
    if (digitalRead(BUTTON_PIN) == !BUTTON_PRESSED_LEVEL)
    {
      PREV_BUTTON_STATE = !BUTTON_PRESSED_LEVEL;
    }
  }
  return false;
}

void loop() {
  
  if (button_pressed())
  {
    scanBLEDevices();
    if(press_kind == Button::LONG_PRESS)
    {
        LightInfo *all_room_lights = nearer_lights->all_lights();
        bool successful = true;
        if(nearer_lights->get_next_light()->status==LightStatus::ON)
        {
          for (int i=0; i< nearer_lights->num_lights; i++)
          {
            successful = successful & send_request(all_room_lights[i].off_event);
          }
          if (successful) 
          {
            nearer_lights->update_all_lights_status(LightStatus::OFF);
            delay(1000);
            grab_lights();
            deluminator_lights += 1;
          }
        }
        else
        {
          for (int i=0; i< nearer_lights->num_lights; i++)
          {
            successful = successful & send_request(all_room_lights[i].on_event);
          }
          if(successful)
          {
            nearer_lights->update_all_lights_status(LightStatus::ON);
            delay(1500);
            deluminator_lights -= 1;
            return_lights();
          }
        }
      }
      else if(press_kind == Button::SHORT_PRESS)
      {
        LightInfo *next_light = nearer_lights->get_next_light();
        if(next_light->status == LightStatus::ON)
        {
          if(send_request(next_light->off_event))
          {
            next_light->status = LightStatus::OFF;
            delay(1200);
            grab_lights();
            deluminator_lights += 1;
          }
          else
          {
            Serial.println("Light toggle failed");
          }
        }
        else if (next_light->status == LightStatus::OFF)
        {
          if(send_request(next_light->on_event))
          {
            next_light->status = LightStatus::ON;
            delay(1000);
            deluminator_lights -= 1;
              return_lights();
          }
          else
          {
            Serial.println("Light toggle failed");
          }
        }
      }
  }

  // Every 10 seconds, check if we are still connected to wifi.. 
  if (millis() % 30000 == 0 && WiFi.status() != WL_CONNECTED)
  { // ..and connect if we aren't
    connect_to_wifi();
    
  }
}

void blink_lights(uint32_t color)
{
  for(int i=0; i < 3; i++)
  {
    strip.setPixelColor(0, color); //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(200); 
    strip.clear();         //  Clear pixel's color (in RAM)
    strip.show();          //  Update strip to match
    delay(200);
  }
}

void grab_lights()
{
  strip.setPixelColor(1, strip.Color(0,0,0,255)); //  Set pixel's color (in RAM)
  strip.show();                          //  Update strip to match
  delay(250);                           //  Pause for a moment
  for(float rad = 1.57; rad <= 3.14; rad += 0.04)
  { // Fade out 1
    // float w = sin(rad)*255;
    strip.setPixelColor(1, strip.Color(0,0,0,uint8_t(sin(rad)*255)));
    if(deluminator_lights <= 0)
    {
      // Fade in 0
    strip.setPixelColor(0, strip.Color(0,0,0,uint8_t(cos(rad)*-255)));
    }
    delay(50);
    strip.show();
  }
  // Make sure 0 is cleared
  strip.setPixelColor(1, strip.Color(0,0,0,0));
  strip.show();
}

void return_lights()
{
  // Fade out 
  for(float rad = 1.57; rad <= 3.14; rad += 0.04)
  { 
    if(deluminator_lights <= 0)
    {// Fade out 0
      strip.setPixelColor(0, strip.Color(0,0,0,uint8_t(sin(rad)*255)));
    }
    // Fade in 1
    strip.setPixelColor(1, strip.Color(0,0,0,uint8_t(cos(rad)*-255)));
    delay(50);
    strip.show();
  }
  if(deluminator_lights <= 0)
  {
    strip.setPixelColor(0, strip.Color(0,0,0,0));
  }
  for(float rad = 1.57; rad <= 3.14; rad += 0.04)
  { // Fade out 1
    strip.setPixelColor(1, strip.Color(0,0,0,uint8_t(sin(rad)*255)));
    delay(20);
    strip.show();
  }
  strip.setPixelColor(1, strip.Color(0,0,0,0));         //  Clear pixel's color (in RAM)
  strip.show();          //  Update strip to match
  delay(10); 
}

void scanBLEDevices()
{
  if (millis() - last_scanned_timestamp < SCAN_INTERVAL)
  {
    // Assume we're in the same room if last checked within SCAN_INTERVAL seconds
    return;
  }
  BLEDevice::init("");
  setupBLEScan();
  room1_rssi = -99;
  room2_rssi = -99;
  rooms_found = 0;
  Serial.print("Scanning for BLE devices...");
  BLEScanResults foundDevices = pBLEScan->start(SCANTIME, false);
  delay(100);
  pBLEScan->clearResults();
  Serial.println("Done scanning.");
  BLEDevice::deinit();
  last_scanned_timestamp = millis();
}

bool send_request(String event)
{
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client -> setCACert(IFTTT_CERTIFICATE);

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
    
      Serial.print("[HTTPS] begin...\n");
      String request_string = "/trigger/" + event + "/with/key/" + IFTTT_KEY;
      Serial.print("Request string:");
      Serial.println(request_string);
      if (https.begin(*client, SERVER, 443, request_string, true)) {  // HTTPS
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
      // End extra scoping block
    }
    delete client;
    return true;
  } else {
    return false;
  }
}