
// Project - Feather Huzzah version
//  Monitor the garage door with a sensor. If it is open more than openMinutes send a push notification
//
//  Notification is sent by using the Twilio REST API service.
//

#include "GarageMonitor.h"

int doorState(int);
void sendAlert(int);

void setup() {
  Serial.begin(115200);

  pinMode(inputPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);          // Start with LED off

  // Connect to WiFi access point.
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected: "); Serial.println(WiFi.localIP());

  twilio = new Twilio(account_sid, auth_token, fingerprint);

  // pause for serial connection for debugging, don't block in case in production mode
  // On the Huzzah, when this was right after Serial.begin the Wifi connection never worked
  delay(5000);

  // Get the baseline time and door state
  stateChangeTime = now();
  currentState = doorState(inputPin);
  previousState = currentState;

  sendAlert(READY);
}

void loop() {
  currentState = doorState(inputPin);
  // If the state changed, & door had been opened > openMinutes send a SMS
  // Set the previousState and note when the time when the state changed
  if (currentState != previousState) {
    if ((previousState == doorOpen)  && ((now() - stateChangeTime) > (openMinutes * 60))) {
      // State went from Open to Closed
      alarmSent = false;
      sendAlert(G_CLOSED);
    }
    previousState = currentState;
    stateChangeTime = now();
  }

  // If the state is open, see if it has been open long enough to sound the alarm
  if (currentState == doorOpen) {
    if ((now() - stateChangeTime) > (openMinutes * 60) && !alarmSent) {
      sendAlert(OPEN);
      alarmSent = true;
    }
  }

  /*if ((minute() == 0) && (second() == 0)) {
    // Send a heartbeat message to AIO once an hour
    aio_Heartbeat();
  }*/
  delay(pollTime);    // Throttle loop speed, should be at least 1 second
}

// Returns the current state of the door, debounced for debounceDelay ms
// If we don't do this and get bouncy state changes we would end up resetting the countdown of stateMinutes erroneously
int doorState (int pin) {
  boolean newState;
  boolean initialState;
  int debounceDelay = 10;    // ms

  initialState = digitalRead(pin);
  // Debounce logic - state must stay the same for debounceDelay ms or considered noise
  for (int count; count < debounceDelay; count++) {
    delay(1);
    newState = digitalRead(pin);
    // If the state changed then start over
    if (newState != initialState) {
      count = 0;
      initialState = newState;
    }
  }
  //Serial.print("Door State "); Serial.println(initialState);
  if (initialState == 0) {
    // Door is closed
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
  return initialState;
}

void sendAlert(int alertType) {
  switch (alertType) {
    case READY:
                messageBody = "Guardian Ready";
                break;
    case OPEN:
                messageBody = "Garage is Open!";
                break;
    case G_CLOSED:
                messageBody = "Garage has Closed";
                break;
    default:
//                Serial.println("Invalid alertType");
                break;
  }
  Serial.println(messageBody);

  // Response will be filled with connection info and Twilio API responses
  String response;
  bool success = twilio->send_message(
        to_number, from_number,
        messageBody,
        response,
        media_url
  );

  if (success) {
    Serial.println("Success sending SMS");
  } else {
    Serial.println(response);
    Serial.println("POST to twilio failed: ");
  }
}
