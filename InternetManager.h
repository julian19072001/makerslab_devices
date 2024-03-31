/**
 * The internet manager is responsible for setting up the Wi-Fi connection and handling the MQTT connection.
 */

#ifndef INTERNET_MANAGER_H
#define INTERNET_MANAGER_H

#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <AsyncMqtt_Generic.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include "Constants.h"

class InternetManager {
public:
    /**
     * Create a new instance of the InternetManager.
     */
    InternetManager() = default;

    /**
     * Whether the device is currently connecting to Wi-Fi.
     */
    bool isConnectingToWifi = false;

    /**
     * Initialize the Wi-Fi connection through the Wi-Fi manager and set up the MQTT connection.
     */
    void initialize() {
      wifiConnectHandler = WiFi.onStationModeGotIP([this](const WiFiEventStationModeGotIP &event) {
          onWifiConnect(event);
      });

      wifiDisconnectHandler = WiFi.onStationModeDisconnected([this](const WiFiEventStationModeDisconnected &event) {
          onWifiDisconnect(event);
      });

      mqttClient.onConnect([this](bool sessionPresent) { onMqttConnect(); });
      mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason) { onMqttDisconnect(reason); });
      mqttClient.onSubscribe([](const uint16_t &packetId, const uint8_t &qos) { onMqttSubscribe(packetId, qos); });
      mqttClient.onMessage([this](char *topic, char *payload, const AsyncMqttClientMessageProperties &properties,
                                  const size_t &len, const size_t &index, const size_t &total) {
          onMqttMessage(topic, payload, properties, len, index, total);
      });

      mqttClient.setServer(MQTT_HOST, MQTT_PORT);
      mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
      mqttClient.setClientId(MQTT_CLIENT_ID);

      connectToWifi();
    }

    /**
      * Publishes the sensor value to the MQTT topic
      */
    void publishValue(float value) {
      // Clear the JSON buffer
      jsonBuffer.clear();

      // Create a JSON object
      JsonObject root = jsonBuffer.to<JsonObject>();

      // Set the sensor value in the JSON object
      root["sensorValue"] = value;

      // Convert the JSON object to a string
      String response;
      serializeJson(root, response);

      // Publish the JSON string to an MQTT topic using the PubSubClient library
      // The topic is specified as TOPIC_AIR_PRESSURE_CURRENT, and the message is the serialized JSON string
      mqttClient.publish(TOPIC_CURRENT, 0, false, response.c_str());
      Serial.println("[MQTT] Published message to topic: " + String(TOPIC_CURRENT));
    }

    /**
      * Uploads sensor value to the API.
      */
    void uploadValue(float value) {
      // Initialize a wi-fi client & http client
      WiFiClient client;
      HTTPClient httpClient;

      // Create  a json object of the data we want to send
      DynamicJsonDocument doc(INITIAL_JSON_CAPACITY);
      doc["value"] = value;

      String json;
      serializeJson(doc, json);

      Serial.print(json);

      // Setup the request.
      strcpy(url, TUNNEL_URL);
      strcat(url, API_ENDPOINT);

      httpClient.begin(client, url);
      httpClient.addHeader("Content-Type", "application/json");

      // Send the request to the server.
      int httpResponseCode = httpClient.POST(json);

      // Check the response.
      if (httpResponseCode == HTTP_CODE_CREATED) {
        String response = httpClient.getString();
        Serial.println(response);
      } else {
        Serial.println("Error on HTTP request. HTTP response code: " + String(httpResponseCode));
      }
      httpClient.end();
    }

    /**
     * Activate the alarm by sending a message on TOPIC_ALARM_SET.
     * If the alarm is already on or has been turned off by the user, this method will do nothing.
     */
    void activateAlarm() {
      // If the alarm is not already on, send a message to turn it on
      // Do not send a message if the alarm already has been turned on by this device
      if (!alarmCurrentlyOn && !alarmTriggeredByDevice) {
        jsonBuffer.clear();
        JsonObject root = jsonBuffer.to<JsonObject>();
        root["airPressureAlarmOn"] = true;
        String response;
        serializeJson(root, response);

        mqttClient.publish(TOPIC_ALARM_SET, 0, false, response.c_str());
        Serial.println("[MQTT] Published message to topic: " + String(TOPIC_ALARM_SET));
      }
    }

    /**
     * Deactivate the alarm by sending a message on TOPIC_ALARM_SET.
     * If the alarm is already off or has been turned on by the user, this method will do nothing.
     */
    void deactivateAlarm() {
      // Reset the alarm triggered state
      alarmTriggeredByDevice = false;

      // If the alarm is currently on, send a message to turn it off
      if (alarmCurrentlyOn) {
        jsonBuffer.clear();
        JsonObject root = jsonBuffer.to<JsonObject>();
        root["airPressureAlarmOn"] = false;
        String response;
        serializeJson(root, response);

        mqttClient.publish(TOPIC_ALARM_SET, 0, false, response.c_str());
        Serial.println("[MQTT] Published message to topic: " + String(TOPIC_ALARM_SET));
      }
    }

private:
  /**
    * Whether the alarm is currently on (as reported by the alarm via MQTT).
    */
  bool alarmCurrentlyOn = false;

  /**
    * Whether the alarm has been activated by this device.
    */
  bool alarmTriggeredByDevice = false;

  /**
   * Wi-Fi client to use for the MQTT client.
   */
  WiFiClientSecure wiFiClient;

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
   * Buffer to use for parsing and creating JSON messages.
   */
  DynamicJsonDocument jsonBuffer = DynamicJsonDocument(INITIAL_JSON_CAPACITY);

  // bool airPressureAlarmStatus = false;

  char url[100];

  /**
   * Connect to Wi-Fi. If the ESP was set up before it will use the saved credentials.
   * If not it will create an access point with the name and password defined in the constants.
   * This access point must be used to set up the connection with a Wi-Fi network.
   * To set up this connection go to gateway IP (192.1769.4.1) while connected to this access point.
   */
  void connectToWifi() {
    Serial.println("Connecting to Wi-Fi...");

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
    (void) event;
    connectToMqtt();
  }

  /**
   * Reconnect to the MQTT broker after a disconnection.
   * 
   * @param event The event that triggered this callback
   */
  void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    (void) event;
    Serial.println("Disconnected from Wi-Fi.");
    mqttReconnectTimer.detach();  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    if (!isConnectingToWifi) {
        wifiReconnectTimer.once(2, []() {
            EspClass::reset();
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
      mqttClient.subscribe(TOPIC_ALARM_STATUS, 0);

      // Trigger TOPIC_ALARM_SET so that handleAlarmStatus is called
      mqttClient.publish(TOPIC_ALARM_SET, 0, false, "");
  }

  /**
   * Reconnect to the MQTT broker after a disconnection.
   *
   * @param reason The reason for the disconnection
   */
  void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
      (void) reason;

      Serial.print("Disconnected from MQTT. Reason: ");
      Serial.println((int) reason);

      if (WiFi.isConnected()) {
          mqttReconnectTimer.once(2, [this]() { connectToMqtt(); });
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
  void
  onMqttMessage(char *topic, char *payload, const AsyncMqttClientMessageProperties &properties, const size_t &len,
                const size_t &index, const size_t &total) {
      (void) payload;

      Serial.print("[MQTT] Message arrived in topic: ");
      Serial.println(topic);

      // Convert the topic and payload to strings
      String topicString = String(topic);

      // Check if the topic is a topic that should be handled by comparing the topic string with the constants strings
      if (topicString == TOPIC_ALARM_STATUS) {
        handleAlarmStatus(payload);
      } else {
        Serial.println("Unknown topic - ignoring message");
      }
  }

  /**
     * Handle the message received on TOPIC_ALARM_STATUS.
     * Store the alarm status in the alarmCurrentlyOn variable.
     * If the alarm is currently on, set the alarmTriggeredByDevice variable to true.
     * If the payload is invalid, ignore the message.
     *
     * @param payload  The payload of the message
     */
    void handleAlarmStatus(char *payload) {
        // Parse the payload
        jsonBuffer.clear();
        deserializeJson(jsonBuffer, payload);

        // Check if payload contains keys for alarm status
        if (!jsonBuffer["airPressureAlarmOn"].is<bool>()) {
            Serial.println("Invalid payload - ignoring message");
            return;
        }

        // Store the alarm status
        alarmCurrentlyOn = jsonBuffer["airPressureAlarmOn"];

        // If the alarm is currently on, set the alarmTriggeredByDevice variable to true
        if (alarmCurrentlyOn) {
            alarmTriggeredByDevice = true;
        }
    }
};


#endif // INTERNET_MANAGER_H
