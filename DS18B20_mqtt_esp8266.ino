/*
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SLEEPDURATION 60 // secondes
#define NB_TRYWIFI 10 // nbr d'essai connexion WiFi, number of try to connect WiFi

const char* WLAN_SSID = "x";
const char* WLAN_PASSWD = "x";
const char* MQTT_SERVER = "x";
const char* MQTT_USER = "x";
const char* MQTT_PWD = "x";

WiFiClient espClient;
PubSubClient client(espClient);

String MACStr;

OneWire  onew(2); //Broche D4
DallasTemperature DS18XXX(&onew);
DeviceAddress SensorsAddr[3];
String SensorsAddrStr[3];
float SensorsTemp[3];

String DeviceAddress2String(DeviceAddress deviceAddress)
{
  char straddr[16];
  sprintf(straddr, "%02X%02X%02X%02X%02X%02X%02X%02X", deviceAddress[0], deviceAddress[1], deviceAddress[2], deviceAddress[3], deviceAddress[4], deviceAddress[5], deviceAddress[6], deviceAddress[7]);
  String tmp(straddr);  
  return tmp;
}
void setup() 
{
  //---------------------------------------------------------------
  // Init WIFI et MQTT
  //---------------------------------------------------------------   
  //Serial.begin(115200);

  delay(10);

  MACStr = WiFi.macAddress();

  // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent( false );
  
  WiFi.mode( WIFI_STA );
  WiFi.begin( WLAN_SSID, WLAN_PASSWD );

  int _try = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    //Serial.print(".");
    _try++;
    
    if ( _try >= NB_TRYWIFI )
    {
      //Impossible to connect WiFi network, go to deep sleep
      ESP.deepSleep(SLEEPDURATION * 1000000);
    }    
  }

  randomSeed(micros());

  client.setServer(MQTT_SERVER, 1883);
  //---------------------------------------------------------------  


  //---------------------------------------------------------------
  // Lecture température
  //---------------------------------------------------------------
  DS18XXX.begin();
  
  int available = DS18XXX.getDeviceCount();
  DS18XXX.requestTemperatures(); 
  
  for(int x = 0; x != available; x++)
  {
    if(DS18XXX.getAddress(SensorsAddr[x], x))
    {
      SensorsAddrStr[x] = DeviceAddress2String(SensorsAddr[x]);
      
      SensorsTemp[x] = DS18XXX.getTempC(SensorsAddr[x]);

      //Serial.println(SensorsAddrStr[x]);
      //Serial.println(SensorsTemp[x]);
    }
  }
  //---------------------------------------------------------------


  //---------------------------------------------------------------
  // MQTT
  //---------------------------------------------------------------
  String topic;
  String payload;

  topic = "DS18B20";
  topic += "/";
  topic += SensorsAddrStr[0];
  payload = SensorsTemp[0];
    
  if (!client.connected())
  {
    //Serial.print("Attempting MQTT connection...");
    
    if (client.connect(MACStr.c_str(), MQTT_USER, MQTT_PWD))
    {
      //Serial.print("Publish message: ");
      //Serial.println(topic);
      //Serial.println(payload);
      client.publish(topic.c_str(), payload.c_str());
    }
    else
    {
      
    }
  }

  client.loop();

  client.disconnect();

  ESP.deepSleep(SLEEPDURATION * 1000000);
  //---------------------------------------------------------------       
}


void loop()
{
}
