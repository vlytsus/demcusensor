//******************************
 //*Abstract: Read value of PM1,PM2.5 and PM10 of air quality
 //
 //******************************
#include <Arduino.h>
#include <ESP8266WiFi.h>


#define MSG_LENGTH 31   //0x42 + 31 bytes equal to 32 bytes
#define DIGITAL_PIN 8   // Digital pin to be write

unsigned char buf[MSG_LENGTH];

int PM01Value=0;   //define PM1.0 value of the air detector module
int PM2_5Value=0;  //define PM2.5 value of the air detector module
int PM10Value=0;   //define PM10 value of the air detector module

const char* ssid     = "Mokkula-2111";
const char* password = "10085999";
const char* host = "api.thingspeak.com";
const int   SLEEP_TIME = 1 * 60 * 1000;


//GET https://api.thingspeak.com/update?api_key=EG2A8TGZB9O831VJ&field1=0

boolean validateMsg(){
  int receiveSum=0;
  for(int i=0; i<(MSG_LENGTH-2); i++){
    receiveSum=receiveSum + buf[i];
  }
  receiveSum=receiveSum + 0x42;
  return receiveSum == ((buf[MSG_LENGTH-2]<<8) + buf[MSG_LENGTH-1]);
}

int decodePM01(unsigned char *thebuf)
{
  int PM01Val;
  PM01Val=((thebuf[3]<<8) + thebuf[4]); //count PM1.0 value of the air detector module
  return PM01Val;
}

//transmit PM Value to PC
int decodePM2_5(unsigned char *thebuf)
{
  int PM2_5Val;
  PM2_5Val=((thebuf[5]<<8) + thebuf[6]);//count PM2.5 value of the air detector module
  return PM2_5Val;
  }

//transmit PM Value to PC
int decodePM10(unsigned char *thebuf)
{
  int PM10Val;
  PM10Val=((thebuf[7]<<8) + thebuf[8]); //count PM10 value of the air detector module  
  return PM10Val;
}

void powerOnSensor(){
  
  digitalWrite(D0, HIGH);
}

void powerOffSensor(){
  digitalWrite(D0, LOW);
  WiFi.disconnect();
  Serial.println("going to sleep zzz...");
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

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(9600);   //use serial0  
  Serial.setTimeout(1500);    //set the Timeout to 1500ms, longer than the data transmission periodic time of the sensor
  Serial.println("");
  Serial.println(" Initialization started");  
  pinMode(D0, OUTPUT);
  
 //   
 Serial.println("");
 Serial.println("Initialization finished");  
  
}

void sendDataToCloud(){

}

void loop(){
  Serial.println("loop start");
  
  powerOnSensor();
  setupWIFI();
  
  //todo enable fan
  Serial.println("sensor warm-up 30s");
  delay(30 * 1000);

  
  for(int i = 0; i < 100; i++){
    if(Serial.find(0x42)){    //start to read when detect 0x42
      Serial.println("Serial found");
      Serial.readBytes(buf, MSG_LENGTH);
      Serial.println("Serial read");
  
      if(buf[0] == 0x4d && validateMsg()){
        Serial.println("buf validated");
        PM01Value = decodePM01(buf); //count PM1.0 value of the air detector module
        PM2_5Value = decodePM2_5(buf);//count PM2.5 value of the air detector module
        PM10Value = decodePM10(buf); //count PM10 value of the air detector module 

        Serial.print("PM1.0: ");  
        Serial.print(PM01Value);
        Serial.println("  ug/m3");            
      
        Serial.print("PM2.5: ");  
        Serial.print(PM2_5Value);
        Serial.println("  ug/m3");     
        
        Serial.print("PM 10: ");  
        Serial.print(PM10Value);
        Serial.println("  ug/m3");   
        break;
      }else{
        Serial.println("message error");   
      }
    }else{
      Serial.println("sensor communication error");
    }
  }

  sendDataToCloud();
  Serial.println();    
  powerOffSensor();  
}
