#include <SPI.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "epd4in2.h"
#include "epdpaint.h"

#define JSON_DOCUMENT_LENGTH 2048
#define BUFFER_SIZE         256

#define TO_SECONDS(x)       x*1000

#define COLORED     0
#define UNCOLORED   1

#define SPLASH_TIME 5   /* Time the splashscreen shows in seconds */
#define UPDATE_TIME 10  /* Time between updates in seconds */

#define SPLASH_HEADER   "Reserveringen"
#define DEVICE          "Laser 3 98 x 158 cm"  /* Name of device (Make sure its the same as in the supersaas schedule) */

#define TIME_API        "https://worldtimeapi.org/api/timezone/Europe/Amsterdam"

#define API_URL         "https://www.supersaas.com/api/range/"
#define SCHEDULE_ID     "262109"
#define API_KEY         "n-42ScBSfzJPuabzh8Hbaw"

#define TIMESLOTS 18    /* Number of different timeslots that can be booked */
#define BOOKINGS 100   /* Number of bookings you want to request. Make sure it is more or equal to (18*Number of devices) */ 

Epd epd;
HTTPClient http; 
WiFiClientSecure client;

unsigned char image[1250];
Paint paint(image, 32, 300);    //width should be the multiple of 8, This is needed because of the way data gets send to the screen over SPI

char buffer[BUFFER_SIZE];

String timeSlot[TIMESLOTS+2] = {"08:30","08:55","09:20","09:45","10:20","10:45","11:10","11:35","12:00","12:25",
                                "12:50","13:15","13:40","14:05","14:30","14:55","15:20","15:45","16:10","16:30"};

WiFiManager wm;

void setup() {
  Serial.begin(115200);

  epd.Init();
  paint.SetRotate(ROTATE_90); 

  /* Display connecting to WiFi */
  epd.ClearFrame();
  drawWiFi();
  epd.DisplayFrame();

  epd.Sleep();

  client.setInsecure(); // Set client to accept insecure connections (for HTTPS)
  WiFi.mode(WIFI_STA);
  wm.autoConnect(DEVICE,"Password"); // password protected ap

  epd.Init();

  /* Display the splash screen */
  epd.ClearFrame();
  drawSplash();
  epd.DisplayFrame();

  delay(TO_SECONDS(SPLASH_TIME));

  /* Display an empty schedule */
  epd.ClearFrame();
  drawTimes();
  epd.DisplayFrame();
  epd.Sleep();
} 

void loop() {
  drawReservations();
  
  delay(TO_SECONDS(UPDATE_TIME));
}

/* Put text on one of the predefined lines */
void drawOnLine(const char* text, int line, int postionOnLine){
  static int lineNumber[TIMESLOTS+2] = {304, 288, 272, 256, 240, 224, 208, 192, 176, 160, 144, 128, 112, 96, 80, 64, 48, 32, 16, 0};

  paint.SetWidth(16);
  paint.SetHeight(300 - postionOnLine);

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, text, &Font12, COLORED);
  epd.SetPartialWindow(paint.GetImage(), lineNumber[line], postionOnLine, paint.GetWidth(), paint.GetHeight());

  paint.SetHeight(300);
}

/* Draw "Connecting to WiFi" with mac-address */
void drawWiFi(){
  paint.Clear(UNCOLORED);
  paint.DrawStringAt((160 - (float)((sizeof("Connecting to WiFi")*14)/2)), 8, "Connecting to WiFi", &Font20, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 200, 0, paint.GetWidth(), paint.GetHeight());

  byte mac[6];
  WiFi.macAddress(mac);

  snprintf(buffer, sizeof(buffer), "MAC-%2X:%2X:%2X:%2X:%2X:%2X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  Serial.println(WiFi.macAddress());

  paint.Clear(COLORED);
  paint.DrawStringAt((160 - (float)((22*14)/2)), 8, buffer, &Font20, UNCOLORED);
  epd.SetPartialWindow(paint.GetImage(), 168, 0, paint.GetWidth(), paint.GetHeight());
}

/* Draw splash screen with: Splash header and device name */
void drawSplash(){
  paint.Clear(UNCOLORED);
  paint.DrawStringAt((160 - (float)((sizeof(SPLASH_HEADER)*17)/2)), 4, SPLASH_HEADER, &Font24, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 200, 0, paint.GetWidth(), paint.GetHeight());

  int startPosition = (160 - (float)((sizeof(DEVICE)*17)/2));
  if(startPosition < 0) startPosition = 0;
  
  paint.Clear(COLORED);
  paint.DrawStringAt(startPosition, 6, DEVICE, &Font24, UNCOLORED);
  epd.SetPartialWindow(paint.GetImage(), 168, 0, paint.GetWidth(), paint.GetHeight());
}

/* Show all the time slots */
void drawTimes(){
  for(int i = 0; i < 20; i++){
    snprintf(buffer, sizeof(buffer), "%s -", timeSlot[i]);
    drawOnLine(buffer, i, 0);
  }

  drawOnLine("Einde laatste reservering", 18, 49);
  drawOnLine("Sluiting Makerslab", 19, 49);

  paint.SetWidth(24);
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 4, DEVICE, &Font20, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 375, 0, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 8, "De machine is beschikbaar", &Font16, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 351, 0, paint.GetWidth(), paint.GetHeight());
}

/* Function to perform HTTP GET request */
String getReservationData() {
  int httpResponseCode;
  String payload = "0";

  char address[BUFFER_SIZE];
  snprintf(address, sizeof(address), "%s%s.json?api_key=%s&today=true&limit=%d", API_URL, SCHEDULE_ID, API_KEY, BOOKINGS);
  
  http.begin(client, address); // Initialize an HTTP request

  httpResponseCode = http.GET(); // Send an HTTP GET request with data
  payload = http.getString(); // Get the response from the server
  http.end(); // Close the HTTP connection
  return payload;
}

/* Put all the reservations on screen */
void drawReservations(){ 
  static int oldTimeslot;
  int currentTimeslot = 0;

  int currentTime = timeToInt(getTime());
  for(int i = 0; i < TIMESLOTS+1; i++){
    currentTimeslot = i + 1;
    if(currentTime >= timeToInt(timeSlot[i]) && currentTime <= (timeToInt(timeSlot[i + 1]) -1)) break;
    currentTimeslot = 0;
  }

  static String oldReservationData;
  String reservationData = getReservationData();

  if(!strcmp(reservationData.c_str(), oldReservationData.c_str()) && oldTimeslot == currentTimeslot) return;

  oldReservationData = reservationData;
  oldTimeslot = currentTimeslot;

  epd.Init();
  epd.ClearFrame();
  drawTimes();

  DynamicJsonDocument doc(JSON_DOCUMENT_LENGTH);
  DeserializationError error = deserializeJson(doc, reservationData);

  if(error) return;
    
  JsonArray bookings = doc["bookings"];

  if(bookings.size() == 0) return;

  uint8_t booked[TIMESLOTS];
  for(int i = 0; i < TIMESLOTS; i++) booked[i] = false;

  for(JsonObject booking : bookings){
    if(booking["res_name"].as<String>().substring(0) != DEVICE) continue;

    for(int i = 0; i < TIMESLOTS; i++){
      if(!strcmp(booking["start"].as<String>().substring(11).c_str(), timeSlot[i].c_str())){
        drawOnLine(booking["full_name"].as<String>().substring(0).c_str(), i, 56);
        booked[i] = true;
        break;
      }
    }
    
    yield();
  }

  paint.SetWidth(24);

  paint.Clear(UNCOLORED);
  if(!currentTimeslot) paint.DrawStringAt(0, 0, "tijdens openingstijden.", &Font16, COLORED);
  else if(currentTimeslot == (TIMESLOTS + 1)) paint.DrawStringAt(0, 0, "tot: 16:30", &Font16, COLORED);
  else{
    for(int i = currentTimeslot;i < (TIMESLOTS+2); i++){
      if(booked[i] != booked[currentTimeslot - 1]){
        snprintf(buffer, sizeof(buffer), "%s: %s",(booked[currentTimeslot - 1] ? "vanaf" : "tot"), timeSlot[i]);
        paint.DrawStringAt(0, 0, buffer, &Font16, COLORED);
        break;
      }
      if(i == (TIMESLOTS+1)){
        if(booked[TIMESLOTS - 1]) paint.DrawStringAt(0, 0, "vanaf: 16:10", &Font16, COLORED);
        else paint.DrawStringAt(0, 0, "tot: 16:30", &Font16, COLORED);
      }
    }
  }
  epd.SetPartialWindow(paint.GetImage(), 327, 0, paint.GetWidth(), paint.GetHeight());

  epd.DisplayFrame();
  epd.Sleep();
}

String getTime(){
  int httpResponseCode;
  String payload = "0";
  
  http.begin(client, TIME_API); // Initialize an HTTP request

  httpResponseCode = http.GET(); // Send an HTTP GET request with data
  payload = http.getString(); // Get the response from the server
  http.end(); // Close the HTTP connection

  DynamicJsonDocument doc(JSON_DOCUMENT_LENGTH);
  DeserializationError error = deserializeJson(doc, payload);

  if(error) return "0";

  snprintf(buffer, sizeof("00:00"), "%s", doc["datetime"].as<String>().substring(11).c_str());

  return buffer;
}

int timeToInt(String textTime){
  int hours = 0;
  int minutes = 0;
  
  snprintf(buffer, sizeof(buffer), "%s", textTime);
  sscanf(buffer, "%d:%d", &hours, &minutes);

  return ((hours * 100)+minutes);
}