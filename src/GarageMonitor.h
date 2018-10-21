#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "twilio.hpp"
#include <TimeLib.h>
#include "credentials.h"

boolean currentState, previousState;
time_t stateChangeTime;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

Twilio *twilio;

// Optional - a url to an image.  See 'MediaUrl' here:
// https://www.twilio.com/docs/api/rest/sending-messages
String media_url = "";

// Pin assignments
const int ledPin = 2;                 // Blue LED
const int inputPin = 5;               // Digital input pin for the proximity sensor
                                       // Pin 4 on Huzzah wouldn't work (lacks pullup?)
                                       // Pin 2 does not allow sketch uploading
                                       // Tying inputPin to ground yields Door state 0
                                       // Disconnecting inputPin and letting it float yelds Door state 0
                                       // Connecting inputPin to 3V yields Door state 1

const String garageClosed = "closed";
const String garageOpen = "open";

boolean alarmSent = false;            // True if we already sent the Open alarm, don't want to keep sending it

// "Production" values
//const unsigned long oneMinute = 60L*1000L;   // One minute is how often we check to see if door still open/closed
const unsigned long openMinutes = 5;         // Door must be open this many minutes before alert sent

// Debug values
//const unsigned long oneMinute = 15L*1000L;
//const int openMinutes = 2;
//const int closedMinutes = 1;
//const int waitingMinutes = 1;

const int pollTime = 1000;            // How long to sleep between checks for door open/closed: 1000 = 1 second (lots of debug output)
//const int pollTime = 250;
//const int pollTime = 15000;            // How long to sleep between checks for door open/closed: 15000 == 15 seconds

//  These make the code more descriptive and can be swapped for NO/NC sensors
#define doorOpen HIGH
#define doorClosed LOW

// Values for the type of alert to send
const int READY=1;
//const int RUNNING=2;
const int OPEN=3;
const int G_CLOSED=4;

void sendAlert();
int doorState();
