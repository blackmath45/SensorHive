/*
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

  Modification dans la librairie GxEPD :
  fichier libraries\GxEPD\src\GxGDEW0154Z17\GxGDEW0154Z17.cpp
  ligne 472 mettre     "if (micros() - start > 40000000) // >14.9s !" au lieu de "if (micros() - start > 20000000) // >14.9s !"
  Car le display WFT0154CZ17LW met 27 secondes à rafraichir, ceci évite les messages de timeout

  Type de carte : LOLIN(WEMOS) D1 R2 & mini
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//---------------------------------------------------------------
// CONFIGURATION
//---------------------------------------------------------------   
#define SLEEPDURATION 240 // secondes
#define LONGSLEEPDURATION 1800 // secondes
#define NB_TRYWIFI 10 // nbr d'essai connexion WiFi, number of try to connect WiFi
#define NB_WAITMQTT 5
//#define SERIAL_DEBUG

const char* WLAN_SSID = "x";
const char* WLAN_PASSWD = "x";
const char* MQTT_SERVER = "x";
const char* MQTT_USER = "x";
const char* MQTT_PWD = "x";
IPAddress ip(192, 168, 0, 121);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 254);
//---------------------------------------------------------------   

WiFiClient espClient;
PubSubClient client(espClient);

String MACStr;

OneWire  onew(2); //Broche D4
DallasTemperature DS18XXX(&onew);
DeviceAddress SensorsAddr[3];
String SensorsAddrStr[3];
float SensorsTemp[3];

String MQTTReceived;

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
#if defined(SERIAL_DEBUG)
    Serial.begin(115200);
#endif
  
  delay(10);

  MACStr = WiFi.macAddress();
#if defined(SERIAL_DEBUG)
    Serial.println(MACStr);
#endif  

  // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent( false );

  WiFi.setOutputPower(20.5);
  WiFi.config(ip, gateway, subnet);
  WiFi.mode( WIFI_STA );
  WiFi.begin( WLAN_SSID, WLAN_PASSWD );

  int _try = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#if defined(SERIAL_DEBUG)
    Serial.print(".");
#endif   
   
    _try++;
    
    if ( _try >= NB_TRYWIFI )
    {
      //Impossible to connect WiFi network, go to deep sleep
      ESP.deepSleep(LONGSLEEPDURATION * 1000000);
    }    
  }

#if defined(SERIAL_DEBUG)
    Serial.println(WiFi.localIP());
#endif  
  
  randomSeed(micros());

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(MQTTcallback);
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
      
#if defined(SERIAL_DEBUG)
      Serial.println(SensorsAddrStr[x]);
      Serial.println(SensorsTemp[x]);
#endif 

    }
  }
  //---------------------------------------------------------------


  //---------------------------------------------------------------
  // MQTT
  //---------------------------------------------------------------
  String topicTo;
  String topicFrom;
  String payload;

  topicTo = MACStr;
  topicTo += "/";
  topicTo += SensorsAddrStr[0];
  payload = SensorsTemp[0];

  topicFrom = MACStr;
  topicFrom += "/";  
  topicFrom += "SYS";  

  int _tryMQTT = 0;
  
  if (!client.connected())
  {
#if defined(SERIAL_DEBUG)
      Serial.print("Attempting MQTT connection...");
#endif     
    
    if (client.connect(MACStr.c_str(), MQTT_USER, MQTT_PWD))
    {
#if defined(SERIAL_DEBUG)
      Serial.println("Publish message: ");
      Serial.println(topicTo);
      Serial.println(payload);
      Serial.println("Subscribe to: ");
      Serial.println(topicFrom);
#endif      
      MQTTReceived = "";
       
      client.subscribe(topicFrom.c_str());  
      client.publish(topicTo.c_str(), payload.c_str());

	  // A chaque message envoyé au broker, il renvoie l'heure sur le sous topic SYS
      while((_tryMQTT < NB_WAITMQTT ) and (MQTTReceived.length() == 0))
      {
        client.loop();
        delay(1000);
#if defined(SERIAL_DEBUG)
      Serial.println("wait for message.");
#endif             
        _tryMQTT++;
      }
      
    }
  }

  client.disconnect();
  WiFi.disconnect(); 
  WiFi.mode( WIFI_OFF );
  //---------------------------------------------------------------



  //---------------------------------------------------------------
  // Sleep
  //---------------------------------------------------------------  
  ESP.deepSleep(SLEEPDURATION * 1000000);
  //---------------------------------------------------------------       
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) 
{
  
#if defined(SERIAL_DEBUG)
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
#endif    
  for (int i = 0; i < length; i++)
  {
#if defined(SERIAL_DEBUG)    
    Serial.print((char)payload[i]);
#endif       
    MQTTReceived += (char)payload[i];
  }
#if defined(SERIAL_DEBUG)  
  Serial.println();
#endif   

}

void loop()
{
}
