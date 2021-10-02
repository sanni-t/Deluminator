#ifndef DELUMINATOR_H
#define DELUMINATOR_H

#include "deluminator_secrets.h"
#include "roomlights.h"
///////please enter your sensitive data in the Secret tab/deluminator_secrets.h
char SSID[] = SECRET_SSID;        // your network SSID (name)
char PASS[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
const char* IFTTT_KEY = SECRET_IFTTT_KEY;  // your secret key for webhooks
const char* IFTTT_CERTIFICATE = IFTTT_ROOTCA_CERT;
// Change to false if pushbutton is NC (normally connected). 
// Keep true for NO (normally open)
#define PUSHBUTTON_NO true

#if PUSHBUTTON_NO == true
	#define BUTTON_PRESSED_LEVEL LOW
#else
	#define BUTTON_PRESSED_LEVEL HIGH
#endif

#define BUTTON_PIN 	21
#define LIGHT_PIN	14	// Pin on the Arduino connected to the NeoPixel
#define LED_COUNT 2		// How many NeoPixels are attached to the Arduino?

#define TOTAL_ROOMS 2

int KEY_INDEX = 0;            // your network key Index number (needed only for WEP)
int WIFI_STATUS = WL_IDLE_STATUS;
const char* SERVER = "maker.ifttt.com";    // address for IFTTT

enum class Button
{
	NONE,
	SHORT_PRESS,
	LONG_PRESS
};

// Change these to appropriate name & webhook events & default status
LightInfo HALL_LIGHTS[] = {{"hall_light1_on", "hall_light1_off", LightStatus::ON},
					  	   {"hall_light2_on", "hall_light2_off", LightStatus::ON},
					  	   {"hall_light3_on", "hall_light3_off", LightStatus::ON}};

LightInfo ROOM2_LIGHTS[] = {{"room2_light_on", "room2_light_off", LightStatus::ON}};


bool PREV_BUTTON_STATE = !BUTTON_PRESSED_LEVEL;
Button press_kind = Button::NONE;
int deluminator_lights = 0;

/********** BLE STUFF **********/
#define SCANTIME  4 //Time spent scanning for BLE devices. In seconds.
#define SCAN_INTERVAL  30000 //Minimum interval between two BLE scans (milliseconds)
const std::string ROOM_1_BLE = "aa:bb:cc:dd:ee:ff\0";	// MAC address of BLE device in Room1
const std::string ROOM_2_BLE = "11:22:33:44:55:66\0";	// MAC address of BLE device in Room2

unsigned long last_scanned_timestamp = 0;
int room1_rssi = -99;	//default
int room2_rssi = -99;	//default
uint8_t rooms_found = 0;
#endif 