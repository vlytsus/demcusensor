//******************************
// PMS5003 Dust sensor application
// That reads PM1, pm2.5, pm10 values and sends to cloud application
// wia WIFI connection
//******************************
#include <Arduino.h>
#include <ESP8266WiFi.h>

//#define DEBUG
#ifdef DEBUG
 #define DEBUG_PRINTLN(x)  Serial.println(x)
 #define DEBUG_PRINT(x)  Serial.print(x)
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
#endif

// #define CONTINUOUS_MODE //read data continously and print to serial, no sensor deep sleep in continuous mode requred
#define NETWORK_MODE //no network required in debug mode, data will be printed to serial 

#define MSG_LENGTH 31   //0x42 + 31 bytes equal to PMS5003 serial message packet lenght
#define HTTP_TIMEOUT 20000 //maximum http response wait period, sensor disconects if no response
#define MIN_WARM_TIME 35000 //warming-up period requred for sensor to enable fan and prepare air chamber
unsigned char buf[MSG_LENGTH];

int pm01Value = 0;   //define PM1.0 value of the air detector module
int pm2_5Value = 0;  //define pm2.5 value of the air detector module
int pm10Value = 0;   //define pm10 value of the air detector module
int airQualityIndex = 0;

const char* host = "api.thingspeak.com";
const char* CLOUD_APPLICATION_ENDPOINT = "update?api_key=EG2A8TGZB9O831VJ&field1=";
const char* ssid     = "Mokkula-2111";
const char* password = "10085999";
const int   SLEEP_TIME = 3 * 60 * 1000;

//GET https://api.thingspeak.com/update?api_key=EG2A8TGZB9O831VJ&field1=0

boolean validateMsg(){
  int receiveSum=0;
  for(int i=0; i<(MSG_LENGTH-2); i++){
    receiveSum=receiveSum + buf[i];
  }
  receiveSum=receiveSum + 0x42;
  return receiveSum == ((buf[MSG_LENGTH-2]<<8) + buf[MSG_LENGTH-1]);
}

int decodepm01(unsigned char *thebuf){
  return ((thebuf[3]<<8) + thebuf[4]); //count PM1.0 value of the air detector module
}

//transmit PM Value to PC
int decodepm2_5(unsigned char *thebuf){
  return ((thebuf[5]<<8) + thebuf[6]);//count pm2.5 value of the air detector module
}

//transmit PM Value to PC
int decodepm10(unsigned char *thebuf){
  return ((thebuf[7]<<8) + thebuf[8]); //count pm10 value of the air detector module  
}

int decodeAtmosphericPM01(unsigned char *thebuf){
  return ((thebuf[9]<<8) + thebuf[10]); //count PM1.0 atmospheric value of the air detector module
}

//transmit PM Value to PC
int decodeAtmosphericPM2_5(unsigned char *thebuf){
  return ((thebuf[11]<<8) + thebuf[12]);//count pm2.5 atmospheric value of the air detector module
}

//transmit PM Value to PC
int decodeAtmosphericPM10(unsigned char *thebuf){
  return ((thebuf[13]<<8) + thebuf[14]); //count pm10 atmospheric value of the air detector module  
}

// AQI formula: https://en.wikipedia.org/wiki/Air_Quality_Index#United_States
int toAQI(int I_high, int I_low, int C_high, int C_low, int C) {
  return (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low;
}

//thanks to https://gist.github.com/nfjinjing/8d63012c18feea3ed04e
int calculateAQI25(float density) {
 
  int d10 = (int)(density * 10);
  if (d10 <= 0) {
    return 0;
  } else if(d10 <= 120) {
    return toAQI(50, 0, 120, 0, d10);
  } else if (d10 <= 354) {
    return toAQI(100, 51, 354, 121, d10);
  } else if (d10 <= 554) {
    return toAQI(150, 101, 554, 355, d10);
  } else if (d10 <= 1504) {
    return toAQI(200, 151, 1504, 555, d10);
  } else if (d10 <= 2504) {
    return toAQI(300, 201, 2504, 1505, d10);
  } else if (d10 <= 3504) {
    return toAQI(400, 301, 3504, 2505, d10);
  } else if (d10 <= 5004) {
    return toAQI(500, 401, 5004, 3505, d10);
  } else if (d10 <= 10000) {
    return toAQI(1000, 501, 10000, 5005, d10);
  } else {
    return 1001;
  }
}

void powerOnSensor(){
  digitalWrite(D0, HIGH);
}

void powerOffSensor(){
#ifdef NETWORK_MODE
  WiFi.disconnect(); 
#endif
#ifndef CONTINUOUS_MODE
  digitalWrite(D0, LOW);
  //ESP.deepSleep(SLEEP_TIME * 1000000); //deep sleep in microseconds, unfortunately doesn't work properly
  DEBUG_PRINTLN("going to sleep zzz...");
  delay(SLEEP_TIME);
#endif
}

void setupWIFI(){
#ifdef NETWORK_MODE
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");  
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
#endif
}

void sendDataToCloud(){
#ifdef NETWORK_MODE // wifi connection is required only in network mode

  DEBUG_PRINTLN("sendDataToCloud start");  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect("api.thingspeak.com", 80)) {
    DEBUG_PRINTLN("connection failed");
    return;
  }

  //create URI for request
  String url = CLOUD_APPLICATION_ENDPOINT + String(pm01Value) + 
  "&field2=" + String(pm2_5Value) +
  "&field3=" + String(pm10Value) +
  "&field6=" + String(airQualityIndex);
  
  DEBUG_PRINTLN("Requesting GET: " + url);
  // This will send the request to the server
  client.print(String("GET /") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Accept: */*\r\n" +
               "User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n" +
               "Connection: close\r\n" +
               "\r\n");               
  client.flush();
  delay(10);
  DEBUG_PRINTLN("wait for response");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > HTTP_TIMEOUT) {
      DEBUG_PRINTLN(">>> Client Timeout !");
      client.stop();
      DEBUG_PRINTLN("closing connection by timeout");
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    DEBUG_PRINT(line);
  }

  client.stop();
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("closing connection");
#endif
}

void setup(){
  
  Serial.begin(9600);   //use serial0
#ifdef DEBUG
 Serial.println(" Init started: DEBUG MODE");
#else
 Serial.println(" Init started: NO DEBUG MODE");
#endif
  Serial.setTimeout(1500);//set the Timeout to 1500ms, longer than the data transmission time of the sensor
  pinMode(D0, OUTPUT);  
  DEBUG_PRINTLN("Initialization finished");  
}

void loop(){
  DEBUG_PRINTLN("loop start");
  unsigned long timeout = millis();  
   
  powerOnSensor();
  setupWIFI();

#ifndef CONTINUOUS_MODE //no warm-up required for continous mode
  timeout = MIN_WARM_TIME - (millis() - timeout);
  if(timeout > 0){
    DEBUG_PRINT("sensor warm-up: ");
    DEBUG_PRINTLN(timeout);    
    delay(timeout);
  }
#endif

  for(int i = 0; i < 30; i++){
    if(Serial.find(0x42)){    //start to read when detect 0x42
      Serial.readBytes(buf, MSG_LENGTH);
  
      if(buf[0] == 0x4d && validateMsg()){
        //pm01Value = decodepm01(buf); //count PM1.0 value of the air detector module
        //pm2_5Value = decodepm2_5(buf);//count pm2.5 value of the air detector module
        //pm10Value = decodepm10(buf); //count pm10 value of the air detector module 
        
        pm01Value = decodeAtmosphericPM01(buf); //count PM1.0 atmospheric value of the air detector module
        pm2_5Value = decodeAtmosphericPM2_5(buf);//count pm2.5 atmospheric value of the air detector module
        pm10Value = decodeAtmosphericPM10(buf); //count pm10 atmospheric value of the air detector module 

#ifdef DEBUG //debug info only for serial connection debugging
        DEBUG_PRINTLN("PM 1.0: " + String(pm01Value));
        DEBUG_PRINTLN("PM 2.5: " + String(pm2_5Value));
        DEBUG_PRINTLN("PM  10: " + String(pm10Value));
        DEBUG_PRINTLN("AQI   : " + String(airQualityIndex));
        DEBUG_PRINTLN("--------------");
#endif
        if(pm01Value == pm2_5Value || pm2_5Value == pm10Value){
          //it is very rarely happened that different particles have same value, better to read again
          DEBUG_PRINT("skip loop:");
          DEBUG_PRINTLN(i);
          continue;
        }
        airQualityIndex = calculateAQI25(pm2_5Value);
        sendDataToCloud();

#ifdef CONTINUOUS_MODE //loop forever for debugging purposes only
       i = 0;
       delay(1000);
#else
       break; //data processed, exit from for loop and sleep
#endif //for CONTINUOUS_MODE

      }else{
        DEBUG_PRINTLN("message validation error");
      }
    }else{
      DEBUG_PRINTLN("sensor msg start not found");
    }
  }
  
  powerOffSensor();  
}
