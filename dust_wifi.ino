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

#define MAX_UNSIGNED_INT     65535
#define MSG_LENGTH 31   //0x42 + 31 bytes equal to PMS5003 serial message packet lenght
#define HTTP_TIMEOUT 20000 //maximum http response wait period, sensor disconects if no response
#define MIN_WARM_TIME 30000 //warming-up period requred for sensor to enable fan and prepare air chamber
unsigned char buf[MSG_LENGTH];

unsigned int pm01Value = 0;   //define PM1.0 value of the air detector module
unsigned int pm2_5Value = 0;  //define pm2.5 value of the air detector module
unsigned int pm10Value = 0;   //define pm10 value of the air detector module
unsigned int pmRAW25 = 0;

unsigned long timeout = 0;

const char* host = "api.thingspeak.com";
const char* CLOUD_APPLICATION_ENDPOINT = "update?api_key=XXXXXXXXXXXXX=";
const char* ssid     = "Free_WiFi";
const char* password = "***********";
const int   SLEEP_TIME = 5 * 60 * 1000;

struct PMSMessage {
    unsigned int pm1tsi;
    unsigned int pm25tsi;
    unsigned int pm10tsi;
    unsigned int pm1atm;
    unsigned int pm25atm;
    unsigned int pm10atm;
    unsigned int raw03um;
    unsigned int raw05um;
    unsigned int raw10um;
    unsigned int raw25um;
    unsigned int raw50um;
    unsigned int raw100um;
    unsigned int version;
    unsigned int errorCode;    
    unsigned int receivedSum;
    unsigned int checkSum;
};

PMSMessage pmsMessage;

boolean readSensorData(){
  DEBUG_PRINTLN("readSensorData start");
  
  unsigned char ch;
  unsigned char high;  
  unsigned int value = 0;
  pmsMessage.receivedSum = 0;
  
  for(int count = 0; count < 32 && Serial.available(); count ++){    
    ch = Serial.read();
    if ((count == 0 && ch != 0x42) || (count == 1 && ch != 0x4d)) {
      pmsMessage.receivedSum = 0;
      DEBUG_PRINTLN("message failed");
      break;
    }else if((count % 2) == 0){
      high = ch;
    }else{
      value = 256 * high + ch;
    }
    
    if (count == 5) { 
      pmsMessage.pm1tsi = value; //PM 1.0 [ug/m3] (TSI standard)
    } else if (count == 7) {
      pmsMessage.pm25tsi = value; //PM 2.5 [ug/m3] (TSI standard)
    } else if (count == 9) {
      pmsMessage.pm10tsi = value; //PM 10. [ug/m3] (TSI standard)
    } else if (count == 11) {
      pmsMessage.pm1atm = value; //PM 1.0 [ug/m3] (std. atmosphere)
    } else if (count == 13) {
      pmsMessage.pm25atm = value; //PM 2.5 [ug/m3] (std. atmosphere)
    } else if (count == 15) {
      pmsMessage.pm10atm = value; //PM 10. [ug/m3] (std. atmosphere)
    } else if (count == 17) {
      pmsMessage.raw03um = value; //num. particles with diameter > 0.3 um in 100 cm3 of air
    } else if (count == 19) {
      pmsMessage.raw05um = value; //num. particles with diameter > 0.5 um in 100 cm3 of air
    } else if (count == 21) {
      pmsMessage.raw10um = value; //num. particles with diameter > 1.0 um in 100 cm3 of air
    } else if (count == 23) {
      pmsMessage.raw25um = value; //num. particles with diameter > 2.5 um in 100 cm3 of air
    } else if (count == 25) {
      pmsMessage.raw50um = value; //num. particles with diameter > 5.0 um in 100 cm3 of air
    } else if (count == 27) {
      pmsMessage.raw100um = value; //num. particles with diameter > 10. um in 100 cm3 of air
    } else if (count == 29) {
      pmsMessage.version = 256 * high; //version & error code
      pmsMessage.errorCode = ch;      
    }
            
    if (count < 30){
      pmsMessage.receivedSum += ch; //calculate checksum for all bytes except last two      
    } else if (count == 31) {
      pmsMessage.checkSum = value; // last two bytes contains checksum from device
    }
  }

  //read data that is not usefull
  while (Serial.available()){
    Serial.read();
    DEBUG_PRINT(".");
  }

  DEBUG_PRINT("data ready:");
  DEBUG_PRINTLN(pmsMessage.receivedSum);
  DEBUG_PRINTLN(pmsMessage.receivedSum == pmsMessage.checkSum);
  return pmsMessage.receivedSum == pmsMessage.checkSum;
}


void printInfo(){
  //debug printing
#ifdef DEBUG
  DEBUG_PRINT("pm1tsi=");
  DEBUG_PRINT(pmsMessage.pm1tsi);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("pm25tsi=");
  DEBUG_PRINT(pmsMessage.pm25tsi);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("pm10tsi=");
  DEBUG_PRINT(pmsMessage.pm10tsi);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("pm1atm=");
  DEBUG_PRINT(pmsMessage.pm1atm);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("pm25atm=");
  DEBUG_PRINT(pmsMessage.pm25atm);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("pm10atm=");
  DEBUG_PRINT(pmsMessage.pm10atm);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("raw03um=");
  DEBUG_PRINT(pmsMessage.raw03um);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("raw05um=");
  DEBUG_PRINT(pmsMessage.raw05um);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("raw10um=");
  DEBUG_PRINT(pmsMessage.raw10um);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("raw25um=");
  DEBUG_PRINT(pmsMessage.raw25um);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("raw50um=");
  DEBUG_PRINT(pmsMessage.raw50um);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("raw100um=");
  DEBUG_PRINT(pmsMessage.raw100um);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("version=");
  DEBUG_PRINT(pmsMessage.version);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("errorCode=");
  DEBUG_PRINT(pmsMessage.errorCode);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("receivedSum=");
  DEBUG_PRINT(pmsMessage.receivedSum);
  DEBUG_PRINTLN();
  
  DEBUG_PRINT("checkSum=");
  DEBUG_PRINT(pmsMessage.checkSum);
  DEBUG_PRINTLN(); 
#endif
}

void powerOnSensor() {
  digitalWrite(D0, HIGH);
}

void powerOffSensor() {
  WiFi.disconnect();
  digitalWrite(D0, LOW);
  //ESP.deepSleep(SLEEP_TIME * 1000000); //deep sleep in microseconds, unfortunately doesn't work properly
  DEBUG_PRINTLN("going to sleep zzz...");
  delay(SLEEP_TIME);
}

void setupWIFI() {
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  DEBUG_PRINT("connecting to WIFI");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");

    if (millis() - timeout > MIN_WARM_TIME) {
      //can't connect to WIFI for a long time
      //disable sensor to save lazer
      digitalWrite(D0, LOW);
      timeout = millis(); //reset timer
    }
  }
  //enable sensor just in case if was disabled
  digitalWrite(D0, HIGH);

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

void sendDataToCloud() {
  DEBUG_PRINT("emb pm2.5: ");
  DEBUG_PRINTLN(pm2_5Value);
  DEBUG_PRINT("RAW pm2.5: ");
  DEBUG_PRINTLN(pmRAW25);
  
  DEBUG_PRINTLN("sendDataToCloud start");
  
  "&field2=" + String(pm2_5Value) +
               "&field3=" + String(pm10Value) +
               "&field7=" + String(pmRAW25);
  
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
               "&field7=" + String(pmRAW25);

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
  timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > HTTP_TIMEOUT) {
      DEBUG_PRINTLN(">>> Client Timeout !");
      client.stop();
      DEBUG_PRINTLN("closing connection by timeout");
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    DEBUG_PRINT(line);
  }

  client.stop();
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("closing connection");
}

void setup() {
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

void loop() {
  DEBUG_PRINTLN("loop start");
  timeout = millis();

  powerOnSensor();
  setupWIFI();

  timeout = MIN_WARM_TIME - (millis() - timeout);
  if (timeout > 0 && timeout < MIN_WARM_TIME) {
    DEBUG_PRINT("sensor warm-up: ");
    DEBUG_PRINTLN(timeout);
    delay(timeout);
  }

//Max & Min values are used to filter noise data
  unsigned int maxPm01Value =0;
  unsigned int minPm01Value =0;
    
  unsigned int maxPm2_5Value =0;
  unsigned int minPm2_5Value =0;
    
  unsigned int maxPm10Value =0;
  unsigned int minPm10Value =0;
  
  unsigned int maxRAW25 =0;
  unsigned int minRAW25 =0;

  pm01Value = 0;   //define PM1.0 value of the air detector module
  pm2_5Value = 0;  //define pm2.5 value of the air detector module
  pm10Value = 0;   //define pm10 value of the air detector module
  pmRAW25 = 0;   
  int count = 0;
  
  for (int i = 0; i < 35 && count < 8; i++) {
    if(readSensorData()){
        if (pmsMessage.pm25atm == 0 || pmsMessage.pm1atm == pmsMessage.pm25atm || pmsMessage.pm25atm == pmsMessage.pm10atm || pmsMessage.raw25um == 0) {
          //it is very rarely happened that different particles have same value, better to read again
          DEBUG_PRINT("skip loop:");
          DEBUG_PRINTLN(i);
          delay(1000);
          continue;
        }

        //***********************************
        //find max dust per sample
        if (pmsMessage.pm1atm > maxPm01Value) {
          maxPm01Value = pmsMessage.pm1atm;
        }
        //find min dust per sample
        if (pmsMessage.pm1atm < minPm01Value) {
          minPm01Value = pmsMessage.pm1atm;
        }
        pm01Value += pmsMessage.pm1atm;
        //***********************************
        //find max dust per sample
        if (pmsMessage.pm25atm > maxPm2_5Value) {
          maxPm2_5Value = pmsMessage.pm25atm;
        }
        //find min dust per sample
        if (pmsMessage.pm25atm < minPm2_5Value) {
          minPm2_5Value = pmsMessage.pm25atm;
        }
        pm2_5Value += pmsMessage.pm25atm;
        //***********************************
        //find max dust per sample
        if (pmsMessage.pm10atm > maxPm10Value) {
          maxPm10Value = pmsMessage.pm10atm;
        }
        //find min dust per sample
        if (pmsMessage.pm10atm < minPm10Value) {
          minPm10Value = pmsMessage.pm10atm;
        }
        pm10Value += pmsMessage.pm10atm;
        //***********************************        
        //findRAW  max dust per sample
        if (pmsMessage.pm10atm > maxRAW25) {
          maxRAW25 = pmsMessage.raw25um;
        }
        //find min dust per sample
        if (pmsMessage.pm10atm > 0 && pmsMessage.pm10atm < minRAW25) {
          minRAW25 = pmsMessage.raw25um;
        }
        pmRAW25 += pmsMessage.raw25um;
        //***********************************
        printInfo();
        delay(500);
        count++;
    }else{
      delay(1000);//data read failed
    }
  }

  if (count > 2) {
    if (pm2_5Value == 0) {
      pm01Value += minPm01Value;
      pm2_5Value += minPm2_5Value;
      pm10Value += minPm10Value;
      pmRAW25 += minRAW25;
    }

    //remove max & min records from calculations
    pm01Value -= maxPm01Value;
    pm01Value -= minPm01Value;
    //get mid value
    pm01Value = pm01Value / (count - 1);

    //remove max & min records from calculations
    pm2_5Value -= maxPm2_5Value;
    pm2_5Value -= minPm2_5Value;
    //get mid value
    pm2_5Value = pm2_5Value / (count - 1);

    //remove max & min records from calculations
    pm10Value -= maxPm10Value;
    pm10Value -= minPm10Value;
    pm10Value = pm10Value / (count - 1);

    //removve max & min records from calculations
    pmRAW25 -= maxRAW25;
    pmRAW25 -= minRAW25;
    //get mid value
    pmRAW25 = pmRAW25 / (count - 1);
    
    sendDataToCloud();
  }

  powerOffSensor();
}
