/**
 * Main file for the alarm project.
 *
 * This file contains the setup and loop functions.
 */

#include "InternetManager.h"
#include "Constants.h"

InternetManager *internetManager;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    // Create and initialize InternetManager
    internetManager = new InternetManager();
    internetManager->initialize();
}

void loop() {
    int analogValue = analogRead(SENSOR_PIN); // Read the value from the sensor.
    float pressure = ADC_TO_BAR(analogValue); //Convert ADC value to BAR

    // Always call publishValue
    internetManager->publishValue(pressure);

    // Call uploadValue only if pressure is lower than the THRESHOLD
    if (pressure < THRESHOLD) {
      internetManager->uploadValue(pressure);
      internetManager->activateAlarm();
    } else {
      internetManager->deactivateAlarm();
    }

    delay(TWO_SECONDS);
}
