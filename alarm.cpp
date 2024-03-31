/**
 * Main file for the alarm project.
 *
 * This file contains the setup and loop functions.
 */

#include <Arduino.h>
#include "InternetManager.h"
#include "AlarmStateManager.h"
#include "Constants.h"

AlarmStateManager *alarmStateManager = new AlarmStateManager();
InternetManager *internetManager = new InternetManager(alarmStateManager);

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  AlarmStateManager::initialize();
  internetManager->initialize();
}

void loop() {
  internetManager->listenToAlarmDeactivation();
  alarmStateManager->checkTriggerAlarm();
}