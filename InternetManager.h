/**
 * The internet manager is responsible for setting up the Wi-Fi connection and handling the MQTT connection.
 */

#ifndef INTERNET_MANAGER_H
#define INTERNET_MANAGER_H

#include <WiFiManager.h>
#include <PubSubClient.h>

#include <AsyncMqtt_Generic.h>
#include <Ticker.h>

#include <ArduinoJson.h>
#include "Constants.h"
#include "AlarmStateManager.h"

class InternetManager {
public:
  /**
   * Create a new instance of the InternetManager.
   * Initialize the web server on the given port and set the state manager to use for turning the alarm on and off.
   *
   * @param alarmStateManager state manager to use for turning the alarm on and off
   */
  explicit InternetManager(AlarmStateManager *alarmStateManager) {
    this->alarmStateManager = alarmStateManager;
  }

  /**
   * Initialize the Wi-Fi connection through the Wi-Fi manager and set up the MQTT connection.
   */
  void initialize() {
    // Initialize the built-in LED pin as an output and turn it off
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    wifiConnectHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP &event) {
      onWifiConnect(event);
    });

    wifiDisconnectHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected &event) {
      onWifiDisconnect(event);
    });

    mqttClient.onConnect([this](bool sessionPresent) {
      onMqttConnect();
    });

    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
      onMqttDisconnect(reason);
    });

    mqttClient.onSubscribe([](const uint16_t &packetId, const uint8_t &qos) {
      onMqttSubscribe(packetId, qos);
    });

    mqttClient.onMessage([this](char *topic, char *payload, const AsyncMqttClientMessageProperties &properties,
                                const size_t &len, const size_t &index, const size_t &total) {
      onMqttMessage(topic, payload, properties, len, index, total);
    });

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
    mqttClient.setClientId(MQTT_CLIENT_ID);

    connectToWifi();
  }

  unsigned long lastDeactivationTime = 0;

  void listenToAlarmDeactivation() {
    if (digitalRead(ALARM_BUTTON_PIN) == HIGH) {
      if (millis() - lastDeactivationTime >= DELAY_ALARM) {
        activeAlarmTypes.clear();
        setAlarmState("");
        lastDeactivationTime = millis();  // Update the last deactivation time
      }
    }
  }

private:

  /**
   * Wi-Fi client to use for the MQTT client.
   */
  WiFiClient wiFiClient;

  /**
   * Async publish/subscribe client to use for sending messages to the MQTT broker.
   */
  AsyncMqttClient mqttClient;

  /**
   * Wi-Fi event handler to handle a successful connection.
   */
  WiFiEventHandler wifiConnectHandler;

  /**
   * Wi-Fi event handler to handle a disconnection.
   */
  WiFiEventHandler wifiDisconnectHandler;

  /**
   * Timer to use for reconnecting to the MQTT broker.
   */
  Ticker mqttReconnectTimer;

  /**
   * Timer to use for reconnecting to Wi-Fi.
   */
  Ticker wifiReconnectTimer;

  /**
   * State manager to use for turning the alarm on and off.
   */
  AlarmStateManager *alarmStateManager;

  /**
   * Buffer to use for parsing and creating JSON messages.
   */
  DynamicJsonDocument jsonBuffer = DynamicJsonDocument(JSON_BUFFER_SIZE);

  /**
   * Set to store active alarm types
   */
  std::set<std::string> activeAlarmTypes;

  /**
   * Define all alarm keys
   */
  const std::vector<std::string> alarmKeys = {
    KEY_TEST_ALARM_ON,
    KEY_AIRFLOW_ALARM_ON,
    KEY_AIR_PRESSURE_ALARM_ON
  };

  /**
   * Whether the ESP is currently connecting to Wi-Fi.
   */
  bool isConnectingToWifi = false;

  /**
   * Connect to Wi-Fi. If the ESP was set up before it will use the saved credentials.
   * If not it will create an access point with the name and password defined in the constants.
   * This access point must be used to set up the connection with a Wi-Fi network.
   * To set up this connection go to gateway IP (192.1769.4.1) while connected to this access point.
   */
  void connectToWifi() {
    // Initialize the Wi-Fi connection
    Serial.println("Initializing Wi-Fi connection");
    WiFiManager wifiManager;
    isConnectingToWifi = true;
    wifiManager.autoConnect(WIFI_SSID, WIFI_PASSWORD);
    isConnectingToWifi = false;
    Serial.println("Connected to Wi-Fi.");
  }

  /**
   * Connect to the MQTT broker.
   */
  void connectToMqtt() {
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
  }

  /**
   * Connect to the MQTT broker after a successful Wi-Fi connection.
   *
   * @param event The event that triggered this callback
   */
  void onWifiConnect(const WiFiEventStationModeGotIP &event) {
    (void)event;
    connectToMqtt();
  }

  /**
   * Reconnect to the MQTT broker after a disconnection.
   *
   * @param event The event that triggered this callback
   */
  void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    (void)event;
    Serial.println("Disconnected from Wi-Fi.");
    mqttReconnectTimer.detach();  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    if (!isConnectingToWifi) {
      wifiReconnectTimer.once(2, []() {
        EspClass::restart();
      });
    }
  }

  /**
   * Execute the callback function when the client is connected to the MQTT broker.
   */
  void onMqttConnect() {
    Serial.print("Connected to MQTT broker: ");
    Serial.print(MQTT_HOST);
    Serial.print(", port: ");
    Serial.println(MQTT_PORT);

    // Subscribe to topics
    mqttClient.subscribe(TOPIC_PING, 0);
    mqttClient.subscribe(TOPIC_ALARM_SET, 2);
  }

  /**
   * Reconnect to the MQTT broker after a disconnection.
   *
   * @param reason The reason for the disconnection
   */
  void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    (void)reason;

    Serial.print("Disconnected from MQTT. Reason: ");
    Serial.println((int)reason);

    if (WiFi.isConnected()) {
      mqttReconnectTimer.once(2, [this]() {
        connectToMqtt();
      });
    }
  }

  /**
   * Execute the callback function when the client is subscribed to a topic.
   *
   * @param packetId
   * @param qos
   */
  static void onMqttSubscribe(const uint16_t &packetId, const uint8_t &qos) {
    Serial.print("[MQTT] Subscribe acknowledged. PacketId: ");
    Serial.print(packetId);
    Serial.print(". QoS: ");
    Serial.println(qos);
  }

  /**
   * Execute the callback function when a message is received on a subscribed topic.
   *
   * @param topic  The topic the message was received on
   * @param payload  The payload of the message
   * @param properties  The properties of the message
   * @param len  The length of the payload
   * @param index  The index of the message in the payload
   * @param total  The total length of the message
   */
  void onMqttMessage(char *topic, char *payload, const AsyncMqttClientMessageProperties &properties, const size_t &len,
                     const size_t &index, const size_t &total) {
    (void)payload;

    Serial.print("[MQTT] Message arrived in topic: ");
    Serial.println(topic);

    // Convert the topic and payload to strings
    String topicString = String(topic);

    // Check if the topic is a topic that should be handled by comparing the topic string with the constants strings
    if (topicString == TOPIC_PING) {
      handlePing();
    } else if (topicString == TOPIC_ALARM_SET) {
      setAlarmState(payload);
    } else {
      Serial.println("Unknown topic - ignoring message");
    }
  }

  /**
   * Handle the message received on TOPIC_PING.
   * Send a response.
   */
  void handlePing() {
    jsonBuffer.clear();
    JsonObject root = jsonBuffer.to<JsonObject>();
    root["message"] = "Pong!";
    String response;
    serializeJson(root, response);

    mqttClient.publish(TOPIC_PONG, 0, false, response.c_str());
    Serial.println("[MQTT] Published message to topic: " + String(TOPIC_PONG));
  }

  /**
   * Handle the message received on TOPIC_ALARM_SET.
   * Turn the alarm on or off based on the payload and send a response with the current alarm state.
   *
   * The payload should be a JSON object with an "alarmOn" key with a boolean value.
   *
   * @param payload The payload of the message
   */
  void setAlarmState(String payload) {
    // Parse the payload
    jsonBuffer.clear();
    deserializeJson(jsonBuffer, payload);

    // Loop through each alarm key and update alarm type
    for (const auto &alarmKey : alarmKeys) {
      if (jsonBuffer.containsKey(alarmKey) && jsonBuffer[alarmKey].is<bool>()) {
        if (jsonBuffer[alarmKey]) {
          activeAlarmTypes.insert(alarmKey);
        } else {
          activeAlarmTypes.erase(alarmKey);
        }
      }
    }

    // Activate or deactivate the alarm based on the values
    if (!activeAlarmTypes.empty()) {
      alarmStateManager->checkAlarmType(activeAlarmTypes);
    } else {
      alarmStateManager->turnAlarmOff();
    }

    sendAlarmState();
  }

  /**
   * Send a message with the current alarm state.
   *
   * The message will be sent to TOPIC_ALARM_STATUS and will be a JSON object with the following keys:
   * - alarmOn: boolean indicating if the alarm is on
   * - airflowAlarmOn: boolean indicating if the airflow alarm is on
   * - airPressureAlarmOn: boolean indicating if the air pressure alarm is on
   * - testAlarmOn: boolean indicating if the test alarm is on
   */
  void sendAlarmState() {
    jsonBuffer.clear();
    JsonObject root = jsonBuffer.to<JsonObject>();
    root[KEY_ALARM_ON] = alarmStateManager->isAlarmOn();
    root[KEY_AIRFLOW_ALARM_ON] = alarmStateManager->isAlarmOn(KEY_AIRFLOW_ALARM_ON);
    root[KEY_AIR_PRESSURE_ALARM_ON] = alarmStateManager->isAlarmOn(KEY_AIR_PRESSURE_ALARM_ON);
    root[KEY_TEST_ALARM_ON] = alarmStateManager->isAlarmOn(KEY_TEST_ALARM_ON);
    String response;
    serializeJsonPretty(root, response);

    mqttClient.publish(TOPIC_ALARM_STATUS, 2, false, response.c_str());
    Serial.println("[MQTT] Published message to topic: " + String(TOPIC_ALARM_STATUS));
  }
};

#endif  // INTERNET_MANAGER_H
