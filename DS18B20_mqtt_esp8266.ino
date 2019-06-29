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

#define durationSleep 60 // secondes
#define NB_TRYWIFI 10 // nbr d'essai connexion WiFi, number of try to connect WiFi

const char* WLAN_SSID = "x";
const char* WLAN_PASSWD = "x";
const char* mqtt_server = "x";

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

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() 
{
  //---------------------------------------------------------------
  // Init WIFI et MQTT
  //---------------------------------------------------------------  
  pinMode(2, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(2, 0); // set pin to the opposite state
  
  Serial.begin(115200);

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
    Serial.print(".");
    _try++;
    
    if ( _try >= NB_TRYWIFI )
    {
      //Impossible to connect WiFi network, go to deep sleep
      ESP.deepSleep(durationSleep * 1000000);
    }    
  }

  randomSeed(micros());

  digitalWrite(2, 1);

  client.setServer(mqtt_server, 1883);
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

      Serial.println(SensorsAddrStr[x]);
      Serial.println(SensorsTemp[x]);
    }
  }
  //---------------------------------------------------------------


  //---------------------------------------------------------------
  // MQTT
  //---------------------------------------------------------------
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  char payload[10];
  char topic[50];
  String topictmp;
  
  snprintf (payload, 10, "%f", SensorsTemp[0]);
  topictmp = MACStr + "/" + SensorsAddrStr[0];
  topictmp.toCharArray(topic, 50);
  
  Serial.print("Publish message: ");
  Serial.println(topic);
  Serial.println(payload);
  client.publish(topic, payload);
  
  ESP.deepSleep(durationSleep * 1000000);
  //---------------------------------------------------------------       
}


void loop()
{
}
