/**
 * The alarm state manager is responsible for checking if the alarm should be triggered.
 * It is also responsible for turning the alarm on and off.
 */

#ifndef ALARM_STATE_MANAGER
#define ALARM_STATE_MANAGER

#include <Arduino.h>
#include "Constants.h"
#include <set>

class AlarmStateManager {
public:
  /**
   * Create a new alarm state manager.
   */
  AlarmStateManager() = default;

  /**
   * Initialize the necessary pins for the alarm.
   */
  static void initialize() {
    pinMode(ALARM_LIGHT_PIN, OUTPUT);
    pinMode(ALARM_CLAXON_PIN, OUTPUT);
    pinMode(ALARM_BUTTON_PIN, INPUT);
  }
  
  /**
   * Check if the alarm should be triggered and trigger it if it should.
   * Else, turn the light off.
   */
  void checkTriggerAlarm() {
    if (testAlarmOn || airflowAlarmOn || airPressureAlarmOn) {
      turnLightOn();
      unsigned long currentTime = millis();
      unsigned long timeSinceActivation = currentTime - alarmActivationTime;

      // Only trigger the alarm sound the first 30 seconds after the alarm is activated
      if (!first30SecondsElapsed && timeSinceActivation <= DELAY_30_SECONDS) {
        triggerCorrectAlarmSound();
      }

      // After 10 minutes, play the alarm sound 5 times
      if (timeSinceActivation >= DELAY_10_MINUTES) {
        // Check if the alarm sound has been played 5 times
        if (alarmSoundCounter < PLAYBACK_COUNT) {
          triggerCorrectAlarmSound(true);
          Serial.println("Activating alarm after 10 minutes. Counter: " + String(alarmSoundCounter));
        } else {
          Serial.println("Alarm has been triggered 5 times. Resetting alarm.");
          // Reset the alarm sound counter
          alarmSoundCounter = 0;
          // Reset the alarm activation time to play the alarm again after 10 minutes
          alarmActivationTime = millis();
          // Set the first 30 seconds elapsed flag to true, so the alarm can be triggered again after 10 minutes and not during the first 30 seconds
          first30SecondsElapsed = true;
        }
      }
    } else {
      turnLightOff();
    }
  }

  /**
   * Checks the active alarm types and updates corresponding flags and alarm count.
   *
   * @param activeAlarmTypes A set containing the names of active alarm types.
   */
  void checkAlarmType(const std::set<std::string> &activeAlarmTypes) {
    // Store the current alarm state
    bool oldTestAlarmOn = testAlarmOn;
    bool oldAirflowAlarmOn = airflowAlarmOn;
    bool oldAirPressureAlarmOn = airPressureAlarmOn;

    testAlarmOn = false;
    airflowAlarmOn = false;
    airPressureAlarmOn = false;
    alarmAmount = 0;

    // Iterate through each alarm type in the set
    for (const auto &alarmType : activeAlarmTypes) {
      if (alarmType == KEY_TEST_ALARM_ON) {
        testAlarmOn = true;
        alarmAmount++;
      } else if (alarmType == KEY_AIRFLOW_ALARM_ON) {
        airflowAlarmOn = true;
        alarmAmount++;
      } else if (alarmType == KEY_AIR_PRESSURE_ALARM_ON) {
        airPressureAlarmOn = true;
        alarmAmount++;
      }
    }

    if (!testAlarmOn && !airflowAlarmOn && !airPressureAlarmOn) {
      turnAlarmOff();
    } else {
      // Reset the alarm activation time if the alarm state has changed
      if (oldTestAlarmOn != testAlarmOn || oldAirflowAlarmOn != airflowAlarmOn || oldAirPressureAlarmOn != airPressureAlarmOn) {
        resetAlarmClaxon();
        alarmActivationTime = millis();
        first30SecondsElapsed = false;
      }
    }
  }

  /**
   * Turn the alarm off.
   */
  void turnAlarmOff() {
    testAlarmOn = false;
    airflowAlarmOn = false;
    airPressureAlarmOn = false;
    alarmActivationTime = 0;
    first30SecondsElapsed = false;
    turnLightOff();
    resetAlarmClaxon();
  }

  /**
   * Check if the alarm is on.
   *
   * @param key The key of the alarm to check. If no key is provided, check if any alarm is on.
   * @return true if the alarm is on, false otherwise
   */
  bool isAlarmOn(const std::string &key = "") {
    if (key == KEY_AIR_PRESSURE_ALARM_ON) {
      return airPressureAlarmOn;
    } else if (key == KEY_AIRFLOW_ALARM_ON) {
      return airflowAlarmOn;
    } else if (key == KEY_TEST_ALARM_ON) {
      return testAlarmOn;
    }

    return testAlarmOn || airflowAlarmOn || airPressureAlarmOn;
  }

private:
  /**
   * Whether the first 30 seconds have elapsed since the alarm was triggered.
   * This is used to prevent the alarm from being triggered again for 30 seconds once the 10 minutes have elapsed.
   */
  bool first30SecondsElapsed = false;

  /**
   * Whether the alarm types are on.
   */
  bool testAlarmOn = false;
  bool airflowAlarmOn = false;
  bool airPressureAlarmOn = false;

  /**
   * The amount of alarms that are on.
   */
  int alarmAmount = 0;

  /**
   * The last time the alarm sound was triggered.
   */
  unsigned long lastAlarmSoundTime = 0;

  /**
   * The amount of times the claxon has beeped.
   */
  int alarmBeepCount = ONE_ALARM;

  /**
   * The time of activation of the alarm.
   */
  unsigned long alarmActivationTime = 0;

  /**
   * Helper counter for the alarm sound.
   */
  int alarmSoundCounter = 0;

  /**
   * Turn the light on.
   */
  static void turnLightOn() {
    digitalWrite(ALARM_LIGHT_PIN, HIGH);
  }

  /**
   * Turn the light off.
   */
  static void turnLightOff() {
    digitalWrite(ALARM_LIGHT_PIN, LOW);
  }

  /**
   * Turn the claxon on.
   */
  static void turnClaxonOn() {
    if (digitalRead(ALARM_CLAXON_PIN) == LOW) {
      digitalWrite(ALARM_CLAXON_PIN, HIGH);
    }
  }

  /**
   * Turn the claxon off.
   */
  static void turnClaxonOff() {
    if (digitalRead(ALARM_CLAXON_PIN) == HIGH) {
      digitalWrite(ALARM_CLAXON_PIN, LOW);
    }
  }

  /**
   * Trigger the correct alarm sound based on the alarm type.
   * If there are multiple alarms, trigger the multiple causes alarm sound.
   *
   * @param incrementCounter Whether it should increment the alarmSoundCounter or not. Defaults to false.
   */
  void triggerCorrectAlarmSound(bool incrementCounter = false) {
    // Check if there's only one alarm type
    if (alarmAmount == ONE_ALARM) {
      if (airflowAlarmOn) {
        triggerAlarmSound(AIRFLOW_BEEPS, incrementCounter);

      } else if (airPressureAlarmOn) {
        triggerAlarmSound(AIR_PRESSURE_BEEPS, incrementCounter);

      } else if (testAlarmOn) {
        triggerAlarmSound(TEST_BEEPS, incrementCounter);
      } else {
        return;
      }
    } else if (alarmAmount > ONE_ALARM) {
      triggerAlarmSound(MULTIPLE_CAUSES_BEEPS, incrementCounter);
    }
  }

  /**
   * Trigger the alarm sound based on the pattern.
   * @param pattern The pattern to use for the alarm sound.
   * @param incrementCounter Whether it should increment the alarmSoundCounter or not. Defaults to false.
   */
  void triggerAlarmSound(int pattern, bool incrementCounter = false) {
    unsigned long currentTime = millis();
    // If the last alarm sound has not been played yet, set the last alarm sound time to the current time
    if (lastAlarmSoundTime == 0) {
      lastAlarmSoundTime = currentTime;
    }

    // For the first BEEP_LENGTH milliseconds, turn the claxon on and return
    if (currentTime - BEEP_LENGTH <= lastAlarmSoundTime) {
      return turnClaxonOn();
    }

    // After the first BEEP_LENGTH milliseconds, turn the claxon off
    turnClaxonOff();

    // If the pattern has not been completed yet and the time between beeps has elapsed:
    if (alarmBeepCount < pattern && (currentTime - (BEEP_LENGTH + TIME_BETWEEN_BEEPS_IN_PATTERN) >= lastAlarmSoundTime)) {
      // increment the alarm beep count, set the last alarm sound time to the current time and return
      lastAlarmSoundTime = currentTime;
      alarmBeepCount++;
      return;
    }

    // If the pattern has been completed, reset the alarm claxon and increment the alarm sound counter if necessary
    if (currentTime - (BEEP_LENGTH + DELAY_ALARM) >= lastAlarmSoundTime) {
      if (incrementCounter) alarmSoundCounter++;

      return resetAlarmClaxon();
    }
  }

  /**
   * Reset alarm claxon state.
   */
  void resetAlarmClaxon() {
    turnClaxonOff();
    alarmBeepCount = ONE_ALARM;
    lastAlarmSoundTime = 0;
  }
};


#endif  //ALARM_STATE_MANAGER