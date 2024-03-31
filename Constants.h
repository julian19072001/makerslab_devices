// This file contains all the constants used in the alarm.

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Device configuration
#define SERIAL_BAUD_RATE            115200
#define ALARM_LIGHT_PIN             D0
#define ALARM_CLAXON_PIN            D5
#define ALARM_BUTTON_PIN            D6

// Delays
#define BEEP_LENGTH                   500
#define TIME_BETWEEN_BEEPS_IN_PATTERN 500
#define DELAY_ALARM                   1500
#define DELAY_30_SECONDS              (1000 * 30)
#define DELAY_10_MINUTES              (1000 * 60 * 10)

// Wi-Fi configuration
#define WIFI_SSID                   "make-sense-alarm"
#define WIFI_PASSWORD               "MakeSense2024"

// MQTT credentials
#define MQTT_HOST                   "data.cleanmobilityhva.nl"
#define MQTT_PORT                   20006
#define MQTT_USER                   "make-sense"
#define MQTT_PASSWORD               "MakeSense2024"
#define MQTT_CLIENT_ID              "make-sense-alarm"

// Topics
#define TOPIC_ALARM                 "alarm"
#define TOPIC_PING                  TOPIC_ALARM "/ping"
#define TOPIC_PONG                  TOPIC_ALARM "/pong"
#define TOPIC_ALARM_SET             TOPIC_ALARM "/set"
#define TOPIC_ALARM_STATUS          TOPIC_ALARM "/status"

// Keys
#define KEY_AIR_PRESSURE_ALARM_ON   "airPressureAlarmOn"
#define KEY_AIRFLOW_ALARM_ON        "airflowAlarmOn"
#define KEY_TEST_ALARM_ON           "testAlarmOn"
#define KEY_ALARM_ON                "alarmOn"

// Beep patterns
#define TEST_BEEPS                  1
#define AIR_PRESSURE_BEEPS          2
#define AIRFLOW_BEEPS               3
#define MULTIPLE_CAUSES_BEEPS       4

// Other
#define JSON_BUFFER_SIZE            1024
#define ONE_ALARM                   1
#define PLAYBACK_COUNT              5
#define ONE_BEEP                    1

#endif  //CONSTANTS_H