/*
 * ESP8266 NodeMCU Real Time Data Graph 
 * Updates and Gets data from webpage without page refresh
 * based on something from https://circuits4you.com
 * reorganized and extended by Gus Mueller, April 24 2022
 * now also resets a Moxee Cellular hotspot if there are network problems
 * since those do not include watchdog behaviors
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "Zanshin_BME680.h"  // Include the BME680 Sensor library
 

BME680_Class BME680;  ///< Create an instance of the BME680 class

float altitude(const int32_t press, const float seaLevel = 1013.25);

float altitude(const int32_t press, const float seaLevel) {
  /*!
  @brief     This converts a pressure measurement into a height in meters
  @details   The corrected sea-level pressure can be passed into the function if it is known,
             otherwise the standard atmospheric pressure of 1013.25hPa is used (see
             https://en.wikipedia.org/wiki/Atmospheric_pressure) for details.
  @param[in] press    Pressure reading from BME680
  @param[in] seaLevel Sea-Level pressure in millibars
  @return    floating point altitude in meters.
  */ 
  static float Altitude;
  Altitude = 44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));  // Convert into meters
  return (Altitude);

}

#include "config.h"

int timeOffset = 0;
bool glblRemote = false;
ESP8266WebServer server(80); //Server on port 80

int moxeePowerSwitch = 14;

//ESP8266's home page:----------------------------------------------------
void handleRoot() {

 server.send(200, "text/html", "nothing"); //Send web page
}

void handleWeatherData() {
  double humidityValue;
  double temperatureValue;
  double pressureValue;
  double gasValue;
  String transmissionString = "";

  int32_t humidityRaw;
  int32_t temperatureRaw;
  int32_t pressureRaw;
  int32_t gasRaw;
  int32_t alt;
  
  static char     buf[16];                        // sprintf text buffer
  static uint16_t loopCounter = 0;                // Display iterations

  BME680.getSensorData(temperatureRaw, humidityRaw, pressureRaw, gasRaw); 
      sprintf(buf, "%4d %3d.%02d", (loopCounter - 1) % 9999,  // Clamp to 9999,
          (int8_t)(temperatureRaw / 100), (uint8_t)(temperatureRaw % 100));   // Temp in decidegrees
  Serial.print(buf);
  sprintf(buf, "%3d.%03d", (int8_t)(humidityRaw / 1000),
          (uint16_t)(humidityRaw % 1000));  // Humidity milli-pct
  Serial.print(buf);
  sprintf(buf, "%7d.%02d", (int16_t)(pressureRaw / 100),
          (uint8_t)(pressureRaw % 100));  // Pressure Pascals
  Serial.print(buf);
  alt = altitude(pressureRaw);                                                // temp altitude
  sprintf(buf, "%5d.%02d", (int16_t)(alt), ((uint8_t)(alt * 100) % 100));  // Altitude meters
  Serial.print(buf);
  sprintf(buf, "%4d.%02d\n", (int16_t)(gasRaw / 100), (uint8_t)(gasRaw % 100));  // Resistance milliohms
  Serial.print(buf);

  humidityValue = (double)humidityRaw/1000;
  temperatureValue = (double)temperatureRaw/100;
  pressureValue = (double)pressureRaw/100;
  gasValue = (double)gasRaw/100;

  
  transmissionString = NullifyOrNumber(temperatureValue) + "*" + NullifyOrNumber(pressureValue) + "*" + NullifyOrNumber(humidityValue) + "*" + NullifyOrNumber(gasValue); //using delimited data instead of JSON to keep things simple
  Serial.println(transmissionString);
  //had to use a global, died a little inside
  if(glblRemote) {
    sendRemoteData(transmissionString);
  } else {
    server.send(200, "text/plain", transmissionString); //Send values only to client ajax request
  }
}

String NullifyOrNumber(double inVal) {
  if(inVal == NULL) {
    return "NULL";
  } else {

    return String(inVal);
  }
}

//SETUP----------------------------------------------------
void setup(void){
  pinMode(moxeePowerSwitch, OUTPUT);
  digitalWrite(moxeePowerSwitch, HIGH);
  Serial.begin(115200);
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");
  // Wait for connection
  int wiFiSeconds = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wiFiSeconds++;
    if(wiFiSeconds > 80) {
      Serial.println("WiFi taking too long, rebooting Moxee");
      rebootMoxee();
    }
  }
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/weatherdata", handleWeatherData); //This page is called by java Script AJAX
  server.begin();                  //Start server
  Serial.println("HTTP server started");

  Serial.print(F("- Initializing BME680 sensor\n"));
  while (!BME680.begin(I2C_STANDARD_MODE)) {  // Start BME680 using I2C, use first device found
    Serial.print(F("-  Unable to find BME680. Trying again in 5 seconds.\n"));
    delay(5000);
  }  // of loop until device is located
  Serial.print(F("- Setting 16x oversampling for all sensors\n"));
  BME680.setOversampling(TemperatureSensor, Oversample16);  // Use enumerated type values
  BME680.setOversampling(HumiditySensor, Oversample16);     // Use enumerated type values
  BME680.setOversampling(PressureSensor, Oversample16);     // Use enumerated type values
  Serial.print(F("- Setting IIR filter to a value of 4 samples\n"));
  BME680.setIIRFilter(IIR4);  // Use enumerated type values
  Serial.print(F("- Setting gas measurement to 320\xC2\xB0\x43 for 150ms\n"));  // "�C" symbols
  BME680.setGas(320, 150);  // 320�c for 150 milliseconds
}

//SEND DATA TO A REMOTE SERVER TO STORE IN A DATABASE----------------------------------------------------
void sendRemoteData(String datastring) {
  WiFiClient clientGet;
  const int httpGetPort = 80;
  String url;
  url =  (String)urlGet + "?storagePassword=" + (String)storagePassword + "&locationId=" + locationId + "&mode=saveData&data=" + datastring;
  Serial.print(">>> Connecting to host: ");
  //Serial.println(hostGet);
  if (!clientGet.connect(hostGet, httpGetPort)) {
    Serial.print("Connection failed, moxee rebooted: ");
    rebootMoxee();
    Serial.print(hostGet);
  } else {
   Serial.println(url);
   clientGet.println("GET " + url + " HTTP/1.1");
   clientGet.print("Host: ");
   clientGet.println(hostGet);
   clientGet.println("User-Agent: ESP8266/1.0");
   clientGet.println("Connection: close\r\n\r\n");
   unsigned long timeoutP = millis();
   while (clientGet.available() == 0) {
     if (millis() - timeoutP > 10000) {
      //let's try a simpler connection and if that fails, then reboot moxee
      clientGet.stop();
      if( clientGet.connect(hostGet, httpGetPort)){
       timeOffset = timeOffset + 20000; //in case two probes are stepping on each other, make this one skew a 20 seconds from where it tried to upload data
       clientGet.println("GET / HTTP/1.1");
       clientGet.print("Host: ");
       clientGet.println(hostGet);
       clientGet.println("User-Agent: ESP8266/1.0");
       clientGet.println("Connection: close\r\n\r\n");
       unsigned long timeoutP2 = millis();
       if (millis() - timeoutP2 > 10000) {
        Serial.print(">>> Client Timeout: moxee rebooted: ");
        Serial.println(hostGet);
        rebootMoxee();
        clientGet.stop();
        return;
       }
      }
      clientGet.stop();
      return;
     }
   }
   //just checks the 1st line of the server response. Could be expanded if needed.
   while(clientGet.available()){
     String retLine = clientGet.readStringUntil('\r');
     Serial.println(retLine);
     break; 
   }
  } //end client connection if else             
  Serial.print(">>> Closing host: ");
  Serial.println(hostGet);
  clientGet.stop();
}

void rebootMoxee() {  //moxee hotspot is so stupid that it has no watchdog.  so here i have a little algorithm to reboot it.
  digitalWrite(moxeePowerSwitch, LOW);
  delay(7000);
  digitalWrite(moxeePowerSwitch, HIGH);
  delay(4000);
  digitalWrite(moxeePowerSwitch, LOW);
  delay(4000);
  digitalWrite(moxeePowerSwitch, HIGH);
}

//LOOP----------------------------------------------------
void loop(void){
  long nowTime = millis() + timeOffset;
  if(nowTime - ((nowTime/(1000 * secondsGranularity) )*(1000 * secondsGranularity)) == 0 ) {  //send data to backend server every <secondsGranularity> seconds or so
    glblRemote = true;
    handleWeatherData();
    glblRemote = false;
  }
  //Serial.println(dht.readTemperature());
  server.handleClient();          //Handle client requests
}
