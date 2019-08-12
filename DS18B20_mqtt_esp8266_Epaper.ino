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
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GxEPD.h>
#include <GxGDEW0154Z17/GxGDEW0154Z17.h>  // 1.54" b/w/r 152x152
#include <Fonts/FreeMonoBold18pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

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

WiFiClient espClient;
PubSubClient client(espClient);

String MACStr;

OneWire  onew(2); //Broche D4
DallasTemperature DS18XXX(&onew);
DeviceAddress SensorsAddr[3];
String SensorsAddrStr[3];
float SensorsTemp[3];

// Connexions
// D0 = RST (EPaper) + RST (ESP8266) pour le deepsleep
// D1 = CS
// D2 = Busy violet
// D3 = DC vert
// D4 = OneWire
// D5 = CLK jaune
// D7 = DIN bleu

GxIO_Class io(SPI, /*CS=D1*/ 5, /*DC=D3*/ 0, /*RST=D4*/ -1); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io, /*RST=D4*/ -1, /*BUSY=D2*/ 4); // default selection of D4(=2), D2(=4)

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

#if defined(SERIAL_DEBUG)
      display.init(115200);
#else
      display.init();
#endif
  
  delay(10);

  MACStr = WiFi.macAddress();
#if defined(SERIAL_DEBUG)
    Serial.println(MACStr);
#endif  

  // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent( false );
  
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
  //---------------------------------------------------------------


  //---------------------------------------------------------------
  // Display
  //---------------------------------------------------------------
  display.setRotation(3);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold18pt7b);
  display.setCursor(0, 0);
  display.println();
  //display.println(name);
  display.print(payload);
  display.drawCircle(120, 16, 2, GxEPD_BLACK);
  display.drawCircle(120, 16, 3, GxEPD_BLACK);
  display.println(" C");
  display.setTextColor(GxEPD_RED);
  display.println(MQTTReceived);
  display.update();
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
