/*
Author: Mees Dubbe, Mark delput
Date: April 25, 2023

Modified by Nazlıcan Eren on November 20, 2023
Modified by F.S. Koulen on December 18, 2023

This sketch is used to control a WeMos D1 mini to read out air flow sensor values.


MIT License
Copyright (c) 2023 Team Make Sense, Internet of Things Minor.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Libraries
#include "InternetManager.h"
#include "Constants.h"
#include "ezTime.h"

InternetManager internetManager = InternetManager();

/**
  Toggles a LED on the given pin.
  The code will first check the current value of the pin.
  Sends a HIGH or LOW signal depending on the current value.

  @pinNumber: The pin of the connected LED.
*/
void toggleLED(const uint8_t pinNumber) {
  int led_value = digitalRead(
    pinNumber);                // Checks the current value of the pin and stores it in led_value.
  digitalWrite(pinNumber,
               led_value == 0 ? HIGH : LOW);  // If led_value equals to 0, send a HIGH value and vice-versa.
}

/**
  Display a signal of a flashing LED.

  @pinNumber: The pin of the connected LED.
*/
void connectedSignal(const uint8_t pinNumber) {
  for (int i = 0; i < SIGNAL_COUNT; i++) {
    toggleLED(pinNumber);
    delay(HALF_A_SECOND);
  }
}

/**
 * Initialize the internet connection.
 * Use the LED to signal the connection status.
 */
void initializeInternetConnection() {
  toggleLED(LED_PIN);  // Turn the LED on when starting to wi-fi connection.

  internetManager.initialize();

  connectedSignal(LED_PIN);  // Turn off the LED when a connection is made.
}

// The setup function will run once on start up.
void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT);

  initializeInternetConnection();

  waitForSync(); // Wait for NTP sync (time needs to be set for checking if the alarm may be activated).

  Serial.println();
  Serial.println("UTC:             " + UTC.dateTime());
}


// Continues loop to execute code.
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);

  int analogValue = analogRead(SENSOR_INPUT);
  internetManager.sendValue(analogValue);

  int currentHour = hour();

  // Activate the alarm if the value is below the threshold and the current time is in the operation hours.
  if (analogValue <= AIRFLOW_THRESHOLD && currentHour >= OPERATION_HOUR_START && currentHour < OPERATION_HOUR_END) {
    internetManager.activateAlarm();
    internetManager.uploadData(analogValue);
  } else {
    internetManager.deactivateAlarm();
  }

  delay(TWO_SECONDS);
}