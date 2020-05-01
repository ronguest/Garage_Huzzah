
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
  delay(3000);

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

  twilio = new Twilio(account_sid, auth_token, fingerprint.c_str());

  server.begin();   // For the health checks
  Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());

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
  // Allows a client connection and responds with either open or closed, used for a health check
  WiFiClient client = server.available();
  // wait for a client (web browser) to connect
  if (client)
  {
    Serial.println("\n[Client connected]");
    while (client.connected())
    {
      // read line by line what the client (web browser) is requesting
      if (client.available())
      {
        String line = client.readStringUntil('\r');
        Serial.print(line);
        // wait for end of client's request, that is marked with an empty line
        if (line.length() == 1 && line[0] == '\n')
        {
          client.println(prepareHtmlPage());
          break;
        }
      }
    }

    delay(1); // give the web browser time to receive the data

    // close the connection:
    client.stop();
    Serial.println("[Client disonnected]");
  }

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
    // Update the SHA1 fingerprint for Twilio once an hour
    get_Fingerprint();
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
  // Update Twilio's SHA1 fingerprint from koala
  if (!get_Fingerprint()) {
    Serial.println("Failed to read Twilio fingerprint");
    return;
  }
  // Serial.print("sendAlert fingerprint is: ");
  // Serial.println(fingerprint.c_str());
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
      //  Serial.println("Invalid alertType");
      break;
  }
  Serial.println(messageBody);

  // Response will be filled with connection info and Twilio API responses
  String response;
  bool success = twilio->send_message(
        to_number, from_number,
        messageBody,
        response,
        media_url, fingerprint.c_str()
  );

  if (success) {
    Serial.println("Success sending SMS");
  } else {
    Serial.println(response);
    Serial.println("POST to twilio failed: ");
  }
}

// prepare a web page to be send to a client (web browser)
String prepareHtmlPage()
{
  String htmlPage =
     String("HTTP/1.1 200 OK\r\n") +
            "Content-Type: text/html\r\n" +
            "Connection: close\r\n" +  // the connection will be closed after completion of the response
//"Refresh: 5\r\n" +  // refresh the page automatically every 5 sec
            "\r\n" +
            "<!DOCTYPE HTML>" +
            "<html>" +
            (doorState(inputPin) ? garageOpen : garageClosed) +
            "</html>" +
            "\r\n";
  return htmlPage;
}

boolean get_Fingerprint() {
  WiFiClient client;

  if (!client.connect(koala, 80)) {
    Serial.println("Koala connection failed!\r\n");
    return false;
  }

  // Make a HTTP request:
  String header = "GET /" + fp_file + " HTTP/1.1";
  // Serial.print("header ");
  // Serial.println(header);
  client.println(header);
  String host = "Host: " + koala;
  client.println(host);
  client.println("Connection: close");
  client.println();  

  String response;
  // Current kludge assumes the fingerprint is the last non-empty line of right length
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line.length() >= 59) {
      // Serial.println("Add line to response");
      response = line;
    }
  }

  // Serial.print("response: ");
  // Serial.println(response);
  fingerprint = response;

  client.stop();
  return true;
}
