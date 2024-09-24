#include <SoftwareSerial.h>
#include <Arduino.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

#define SOFT_RX D7  // The ESP8266 pin connected to the TX of the bluetooth module
#define SOFT_TX D6  // The ESP8266 pin connected to the RX of the bluetooth module

SoftwareSerial bluetooth(SOFT_RX, SOFT_TX);

String receivedData = "";
String ssid = "";
String password = "";
String receivedChar;

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert Firebase project API Key
#define API_KEY "AIzaSyD6MpF24UXNtokG3H36wzPLwl-ExCfMORw"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://casaintelli-default-rtdb.firebaseio.com/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

//some important variables
String sValue, sValue2;
bool signupOK = false;
int led1 = 2; //PIN D4 NODEMCU ESP8266 AMICA

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

  pinMode(led1, OUTPUT);
}

void loop() {
  connectWithBluetooth();
  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.getString(&fbdo, "CasaIntelli/Tes")) {
      if (fbdo.dataType() == "string") {
        sValue = fbdo.stringData();
        int a = sValue.toInt();
        Serial.println(a);
        if (a == 1) {
          digitalWrite(led1, HIGH);
        } else {
          digitalWrite(led1, LOW);
        }
      } else {
        Serial.println(fbdo.errorReason());
      }
    }
  }
}

void connectWithFirebase() {
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void connectWithBluetooth() {
  if (bluetooth.available()) { // if there is data coming
    String command = bluetooth.readStringUntil('\n'); // read string until meet newline character

    receivedChar = command;
    Serial.println(receivedChar);

    receivedData += receivedChar;

    if (receivedChar != "") {
      Serial.print("Data yang diterima: ");
      Serial.println(receivedData);

      int separatorIndex = receivedData.indexOf('|');
      if (separatorIndex != -1) {
        ssid = receivedData.substring(0, separatorIndex);
        password = receivedData.substring(separatorIndex + 1);

        ssid.trim();
        password.trim();

        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password: ");
        Serial.println(password);

        // bluetooth.print("1");

        connectToWifi(ssid, password);
      } else {
        Serial.println("Format data salah. Gunakan format: SSID|password");
      }
      receivedData = "";
      bluetooth.flush();  // Clear the buffer to avoid receiving the same data again
    }
  }

  delay(500);
}

void connectToWifi(String ssid, String password) {

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
    delay(1000);
  }

  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to Wi-Fi");
  int numberOfTries = 20; // 20 tries, each with 1 second delay

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    numberOfTries--;
    if (numberOfTries == 0) {
      Serial.println();
      Serial.println("Failed to connect to Wi-Fi");
      bluetooth.flush();
      bluetooth.println(F("0"));
      return;
    }
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  bluetooth.flush();
  bluetooth.println(F("1"));
  connectWithFirebase();
}
