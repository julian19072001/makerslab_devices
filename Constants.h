// This file contains all the constants used in the alarm.

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Device configuration
#define SERIAL_BAUD_RATE            115200
#define SENSOR_PIN                  A0

// Delays
#define HALF_A_SECOND               500
#define ONE_SECOND                  1000
#define TWO_SECONDS                 2000

// Formula
#define ADC_TO_BAR(x)               ((x - 170) / 100.0)

// Wi-Fi configuration
#define WIFI_SSID                   "AP Makerslab AirPressure"
#define WIFI_PASSWORD               "Password"

// API url
#define TUNNEL_URL                  "http://data.cleanmobilityhva.nl:20000"
#define API_ENDPOINT                "/api/air-pressure"

// MQTT credentials
#define MQTT_HOST                   "data.cleanmobilityhva.nl"
#define MQTT_PORT                   20006
#define MQTT_USER                   "make-sense"
#define MQTT_PASSWORD               "MakeSense2024"
#define MQTT_CLIENT_ID              "make-sense-air-pressure"

// Topic air pressure
#define TOPIC_AIR_PRESSURE          "air-pressure"
#define TOPIC_CURRENT               TOPIC_AIR_PRESSURE "/current"

// Topic alarm
#define TOPIC_ALARM                 "alarm"
#define TOPIC_ALARM_SET             TOPIC_ALARM "/set"
#define TOPIC_ALARM_STATUS          TOPIC_ALARM "/status"

// Other
#define INITIAL_JSON_CAPACITY       1024
#define SIGNAL_COUNT                5
#define THRESHOLD                   2

#endif //CONSTANTS_H
