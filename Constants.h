#ifndef CONSTANTS_H
#define CONSTANTS_H

#define AIRFLOW_THRESHOLD   300 // This is the threshold while it is not converted to m/s yet.

// Device configuration
#define BAUD_RATE           115200
#define SENSOR_INPUT        A0
#define RESET_PIN           16
#define SIGNAL_COUNT        5
#define LED_PIN             D5

// Delays
#define TWO_SECONDS         2000
#define HALF_A_SECOND       500

// Wi-Fi configuration
#define WIFI_SSID           "make-sense-airflow"
#define WIFI_PASSWORD       "MakeSense2024"

// API configuration
#define TUNNEL              "http://data.cleanmobilityhva.nl:20000"
#define CONTENT_TYPE        "application/json"
#define AIRFLOW_URL         "/api/airflow"

#define JSON_DOC_SIZE       1024

// MQTT credentials
#define MQTT_HOST                   "data.cleanmobilityhva.nl"
#define MQTT_PORT                   20006
#define MQTT_USER                   "make-sense"
#define MQTT_PASSWORD               "MakeSense2024"
#define MQTT_CLIENT_ID              "make-sense-airflow"

// Topics
#define TOPIC_AIRFLOW               "airflow"
#define TOPIC_PING                  TOPIC_AIRFLOW "/ping"
#define TOPIC_PONG                  TOPIC_AIRFLOW "/pong"
#define TOPIC_AIRFLOW_VALUE         TOPIC_AIRFLOW "/value"

#define TOPIC_AIRFLOW_ALARM         "alarm"
#define TOPIC_ALARM_SET             TOPIC_AIRFLOW_ALARM "/set"
#define TOPIC_ALARM_STATUS          TOPIC_AIRFLOW_ALARM "/status"

// JSON Keys
#define JSON_KEY_AIRFLOW_ALARM_ON   "airflowAlarmOn"

// Operation hours
// The system will only record data and/or activate the alarm between these hours.
// The air extraction system is turned off around 21:30 UTC, except for Fridays, then it is turned off at 17:22 UTC,
// and turned on again around 6:30 UTC, so the operation hours are set to 7:00 UTC to 17:00 UTC.
#define OPERATION_HOUR_START        7
#define OPERATION_HOUR_END          17

#endif //CONSTANTS_H
