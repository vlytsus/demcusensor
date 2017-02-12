//******************************
 //*Abstract: Read value of PM1,PM2.5 and PM10 of air quality
 //
 //******************************
#include <Arduino.h>
#include <ESP8266WiFi.h>

//#define DEBUG
#ifdef DEBUG
 #define DEBUG_PRINTLN(x)  Serial.println (x)
 #define DEBUG_PRINT(x)  Serial.print (x)
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
#endif

#define MSG_LENGTH 31   //0x42 + 31 bytes equal to 32 bytes
#define HTTP_TIMEOUT 20000
#define MIN_WARM_TIME 30000
unsigned char buf[MSG_LENGTH];

int PM01Value=0;   //define PM1.0 value of the air detector module
int PM2_5Value=0;  //define PM2.5 value of the air detector module
int PM10Value=0;   //define PM10 value of the air detector module

const char* ssid     = "Mokkula-2111";
const char* password = "10085999";
const char* host = "api.thingspeak.com";
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

int decodePM01(unsigned char *thebuf){
  return ((thebuf[3]<<8) + thebuf[4]); //count PM1.0 value of the air detector module
}

//transmit PM Value to PC
int decodePM2_5(unsigned char *thebuf){
  return ((thebuf[5]<<8) + thebuf[6]);//count PM2.5 value of the air detector module
}

//transmit PM Value to PC
int decodePM10(unsigned char *thebuf){
  return ((thebuf[7]<<8) + thebuf[8]); //count PM10 value of the air detector module  
}

void powerOnSensor(){
  digitalWrite(D0, HIGH);
}

void powerOffSensor(){
  digitalWrite(D0, LOW);
  WiFi.disconnect();
  
  DEBUG_PRINTLN("going to sleep zzz...");
    
  //ESP.deepSleep(SLEEP_TIME * 1000000);
  delay(SLEEP_TIME);
}

void setupWIFI(){
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");  
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

void setup()
{
  Serial.begin(9600);   //use serial0  
  Serial.setTimeout(1500);    //set the Timeout to 1500ms, longer than the data transmission periodic time of the sensor
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN(" Initialization started");  
  pinMode(D0, OUTPUT);

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("Initialization finished");  
  
}

void sendDataToCloud()
{
  DEBUG_PRINTLN("sendDataToCloud start");  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect("api.thingspeak.com", 80)) {
    DEBUG_PRINTLN("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "update?api_key=EG2A8TGZB9O831VJ&field1=" + String(PM01Value) + 
  "&field2=" + String(PM2_5Value) +
  "&field3=" + String(PM10Value);
  
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
    Serial.print(line);
  }

  client.stop();
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("closing connection");
}

void loop(){
  DEBUG_PRINTLN("loop start");
  unsigned long timeout = millis();

  PM01Value  = 0;
  PM2_5Value = 0;
  PM10Value  = 0;
   
  powerOnSensor();
  setupWIFI();
  
  timeout = MIN_WARM_TIME - (millis() - timeout);
  if(timeout > 0){
    DEBUG_PRINT("sensor warm-up: ");
    DEBUG_PRINTLN(timeout);    
    delay(timeout);
  }
  
  for(int i = 0; i < 20; i++){
    if(Serial.find(0x42)){    //start to read when detect 0x42
      DEBUG_PRINTLN("Serial found");
      Serial.readBytes(buf, MSG_LENGTH);
      DEBUG_PRINTLN("Serial read");
  
      if(buf[0] == 0x4d && validateMsg()){
        DEBUG_PRINTLN("buf validated");
        PM01Value = decodePM01(buf); //count PM1.0 value of the air detector module
        PM2_5Value = decodePM2_5(buf);//count PM2.5 value of the air detector module
        PM10Value = decodePM10(buf); //count PM10 value of the air detector module 

        if(PM01Value == 0 && PM2_5Value == 0 && PM10Value == 0){
          DEBUG_PRINT("000 - skip loop: ");
          DEBUG_PRINTLN(i);
          continue;
        }
        break;
      }else{
        DEBUG_PRINTLN("message error");   
      }
    }else{
      DEBUG_PRINTLN("sensor communication error");
    }
  }

  sendDataToCloud();
  DEBUG_PRINTLN();    
  powerOffSensor();  
}
