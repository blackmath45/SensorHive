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
long lastMsg = 0;
char msg[50];
int value = 0;

OneWire  onew(2); //Broche D4
DallasTemperature DS18XXX(&onew);
DeviceAddress SensorsAddr[3];
String SensorsAddrStr[3];
float SensorsTemp[3];


void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  //WiFi.forceSleepWake();
  //delay( 1 );
  
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
      Serial.println("Impossible to connect WiFi network, go to deep sleep");
      //ESP.deepSleep(durationSleep * 1000000);
    }    
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //WiFi.printDiag(Serial);  
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
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
  pinMode(2, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(2, 1); // set pin to the opposite state
    
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void DeviceAddress2String(DeviceAddress deviceAddress, char *straddr)
{
  sprintf(straddr, "%02X%02X%02X%02X%02X%02X%02X%02X", deviceAddress[0], deviceAddress[1], deviceAddress[2], deviceAddress[3], deviceAddress[4], deviceAddress[5], deviceAddress[6], deviceAddress[7]);
}

void loop()
{
  //---------------------------------------------------------------
  // MQTT
  //---------------------------------------------------------------
/*  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
    ESP.deepSleep(durationSleep * 1000000);
  }*/
  //---------------------------------------------------------------  

  DS18XXX.begin();
  
  int available = DS18XXX.getDeviceCount();
  DS18XXX.requestTemperatures(); 
  
  for(int x = 0; x != available; x++)
  {
    if(DS18XXX.getAddress(SensorsAddr[x], x))
    {
      char straddr[16];
      DeviceAddress2String(SensorsAddr[x], straddr);
      String tmp(straddr);
      SensorsAddrStr[x] = tmp;
      
      SensorsTemp[x] = DS18XXX.getTempC(SensorsAddr[x]);

      Serial.println(SensorsAddrStr[x]);
      Serial.println(SensorsTemp[x]);
    }
  }
}
